use actix::{Actor, AsyncContext, Context, Handler, Message as ActixMessage};
use librevault_core::indexer::reference::ReferenceMaker;
use prost::Message;
use tracing::info;

use crate::bucket::Bucket;
use crate::chunkstorage::actor::{
    Command as MaterializedStorageCommand, Response as MaterializedStorageResponse,
};
use crate::indexer::{IndexingEvent, IndexingTask, Policy};
use crate::object_storage;
use crate::rdb::RocksDBObjectCRUD;

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
                    self.objstore
                        .do_send(object_storage::Command::PutEntity { entity: object });
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
        let (paths_tx, paths_rx) = tokio::sync::oneshot::channel();
        let materialized_storage = self.materialized_storage.clone();
        tokio::spawn(async move {
            let response = materialized_storage
                .send(MaterializedStorageCommand::ListUncommittedPaths)
                .await
                .unwrap();
            let MaterializedStorageResponse::ListDirtyPaths(paths) = response else { panic!() };
            paths_tx.send(paths).unwrap();
        });

        let paths = paths_rx.rec().unwrap();
        for path in paths {
            self.indexer.do_send(IndexingTask {
                policy: Policy::Orphan,
                root: self.root.clone(),
                path,
            });
        }
    }
}
