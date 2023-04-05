use std::collections::HashMap;
use std::fs::File;
use std::io;
use std::io::{BufReader, Read, Seek, SeekFrom};
use std::path::Path;
use std::sync::Arc;

use actix::{Actor, Handler, SyncContext};
use actix_rt::ArbiterHandle;
use librevault_core::indexer::chunk_metadata::ChunkMetadataIndexer;
use librevault_core::indexer::object_metadata::ObjectMetadataIndexer;
use librevault_core::indexer::reference::ReferenceMaker;
use librevault_core::proto::object_metadata::ObjectType;
use librevault_core::proto::{ChunkMetadata, DataStream, ObjectMetadata, ReferenceHash};
use rabin::{Rabin, RabinParams};
use tracing::{debug, span, warn, Level};

use crate::chunkstorage::materialized::MaterializedFolder;
use crate::indexer::{IndexingTaskResult, WrappedIndexingTask};

pub struct IndexerWorker {
    handle: ArbiterHandle,
}

impl IndexerWorker {
    pub fn new(handle: ArbiterHandle) -> Self {
        Self { handle }
    }

    fn make_chunk(
        &self,
        chunk: &[u8],
        ms: Arc<MaterializedFolder>,
        cache: &mut HashMap<ReferenceHash, ChunkMetadata>,
    ) -> ChunkMetadata {
        let chunk_plaintext_refh = ChunkMetadata::compute_plaintext_hash(&chunk);

        if let Some(chunk_md) = cache.get(&chunk_plaintext_refh) {
            debug!("Reusing chunk from same datastream");
            return chunk_md.clone();
        }

        let existing_chunk_md = {
            let (tx, rx) = tokio::sync::oneshot::channel();
            let chunk_plaintext_refh = chunk_plaintext_refh.clone();
            self.handle.spawn(async move {
                let existing_chunk_md = ms
                    .get_chunk_md_by_plaintext_hash(&chunk_plaintext_refh)
                    .await;
                tx.send(existing_chunk_md).unwrap();
            });

            rx.blocking_recv().unwrap()
        };

        let chunk_md = if let Some(existing_chunk_md) = existing_chunk_md {
            debug!("Reusing chunk from DB");
            existing_chunk_md
        } else {
            debug!("Creating new chunk");
            ChunkMetadata::compute_from_buf(&chunk, &[0; 32], 0)
        };

        cache.insert(chunk_plaintext_refh, chunk_md.clone());
        chunk_md
    }

    fn make_datastream<R: Read>(
        &self,
        data: R,
        ms: Arc<MaterializedFolder>,
        size_hint: u64,
    ) -> Result<(Vec<ChunkMetadata>, DataStream), io::Error> {
        let chunker = Rabin::new(data, RabinParams::default());
        let chunks_estimate = (size_hint / chunker.min_size() as u64) as usize;

        let mut cache = HashMap::with_capacity(chunks_estimate);

        let chunk_mds = {
            let mut chunk_mds = Vec::with_capacity(chunks_estimate);
            for chunk in chunker {
                chunk_mds.push(self.make_chunk(&chunk?, ms.clone(), &mut cache))
            }
            chunk_mds
        };

        let datastream = DataStream {
            chunk_md: chunk_mds
                .iter()
                .map(|chunk_md| chunk_md.reference().1)
                .collect(),
        };

        Ok((chunk_mds, datastream))
    }

    fn open_file(path: &Path) -> io::Result<(BufReader<File>, u64)> {
        debug!("Opening file for indexing: {path:?}");
        let mut f = BufReader::new(File::open(path)?);
        let size_hint = f.get_mut().seek(SeekFrom::End(0))?;
        f.get_mut().rewind()?;
        Ok((f, size_hint))
    }
}

impl Actor for IndexerWorker {
    type Context = SyncContext<Self>;
}

impl Handler<WrappedIndexingTask> for IndexerWorker {
    type Result = ();

    fn handle(&mut self, msg: WrappedIndexingTask, _ctx: &mut Self::Context) -> Self::Result {
        let span = span!(
            Level::INFO,
            "IndexerWorker",
            group_id = msg.routing_info.group,
            task_id = msg.routing_info.id
        );
        let _enter = span.enter();

        let Some(result_receiver) = msg.routing_info.result_receiver.upgrade() else {
            warn!("Index result receiver is gone");
            return;
        };

        let task = msg.task;
        let mut object = ObjectMetadata::from_path(&task.path, &task.root, true).unwrap();

        let mut chunks = vec![];
        let mut datastreams = Vec::with_capacity(1);

        if object.r#type == ObjectType::File as i32 {
            let (f, size_hint) = Self::open_file(task.path.as_path()).unwrap();

            let (ds_chunks, datastream) = self.make_datastream(f, msg.ms, size_hint).unwrap();
            chunks.extend(ds_chunks);

            datastreams.push(datastream.clone());

            object.stream = Some(datastream.reference().1);
        }

        let result = IndexingTaskResult {
            routing_info: msg.routing_info,
            object,
            chunks,
            datastreams,
        };
        result_receiver.do_send(result);
    }
}
