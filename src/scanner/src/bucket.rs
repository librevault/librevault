use std::path::PathBuf;
use std::sync::Arc;

use actix::{Actor, Addr, Context, Handler, Message as ActixMessage, WeakAddr};
use libp2p_identity::PeerId;
use librevault_core::indexer::reference::ReferenceMaker;
use prost::Message;
use rocksdb::DB;
use sea_orm::DatabaseConnection;
use tracing::info;

use crate::chunkstorage::materialized::MaterializedFolder;
use crate::datastream_storage::DataStreamStorage;
use crate::directory_watcher::DirectoryWatcher;
use crate::indexer::bucket::BucketIndexer;
use crate::indexer::pool::IndexerWorkerPool;
use crate::indexer::{IndexingEvent, IndexingTask, Policy};
use crate::object_storage::ObjectMetadataStorage;
use crate::rdb::RocksDBObjectCRUD;
use crate::snapshot_storage::SnapshotStorage;

type BucketId = PeerId;

pub struct Bucket {
    snapstore: Arc<SnapshotStorage>,
    dss: Arc<DataStreamStorage>,
    objstore: Arc<ObjectMetadataStorage>,

    watcher: Addr<DirectoryWatcher>,
    indexer_pool: WeakAddr<IndexerWorkerPool>,

    materialized_storage: Arc<MaterializedFolder>,
    indexer: Addr<BucketIndexer>,

    root: PathBuf,
}

impl Bucket {
    pub fn new(
        db: Arc<DB>,
        rdb: Arc<DatabaseConnection>,
        root: PathBuf,
        indexer_pool: WeakAddr<IndexerWorkerPool>,
        addr: WeakAddr<Bucket>,
    ) -> Self {
        let watcher = DirectoryWatcher::new(&root).start();

        let snapstore = Arc::new(SnapshotStorage::new(db.clone()));
        let dss = Arc::new(DataStreamStorage::new(db.clone()));
        let objstore = Arc::new(ObjectMetadataStorage::new(db.clone()));

        let materialized_storage = Arc::new(MaterializedFolder::new(
            &root,
            db.clone(),
            rdb.clone(),
            dss.clone(),
            objstore.clone(),
        ));

        let indexer = BucketIndexer::new(
            indexer_pool.clone().upgrade().unwrap(),
            materialized_storage.clone(),
            addr.clone().recipient(),
        )
        .start();

        Self {
            snapstore,
            dss,
            objstore,

            watcher,
            indexer_pool,

            materialized_storage,
            indexer,

            root,
        }
    }

    pub fn reindex_all(&self) {
        for path in self.materialized_storage.list_actual_paths() {
            self.indexer.do_send(IndexingTask {
                policy: Policy::Orphan,
                root: self.root.clone(),
                path,
            });
        }
    }
}

impl Actor for Bucket {
    type Context = Context<Self>;
}

impl Handler<IndexingEvent> for Bucket {
    type Result = ();

    fn handle(&mut self, msg: IndexingEvent, ctx: &mut Self::Context) -> Self::Result {
        match msg {
            IndexingEvent::SnapshotCreated {
                snapshot,
                objects,
                datastreams,
                chunks,
            } => {
                let refh = snapshot.reference().1;
                info!(
                    "Snapshot created: {:?}, len: {}",
                    refh,
                    snapshot.encoded_len()
                );

                self.snapstore.put_entity(&snapshot);
                for object in objects {
                    self.objstore.put_entity(&object);
                }
                for ds in datastreams {
                    self.dss.put_entity(&ds);
                }
                for chunk in chunks {
                    self.dss.put_entity(&chunk);
                }
            }
        }
    }
}

#[derive(ActixMessage, Debug, Clone)]
#[rtype(result = "()")]
pub struct ReindexAll;

impl Handler<ReindexAll> for Bucket {
    type Result = ();

    fn handle(&mut self, msg: ReindexAll, ctx: &mut Self::Context) -> Self::Result {
        self.reindex_all()
    }
}
