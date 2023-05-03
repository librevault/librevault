use std::sync::Arc;

use actix::{Actor, Context, Handler, Message};
use librevault_core::proto::Snapshot;
use rocksdb::DB;

use crate::rdb::RocksDBObjectCRUD;

#[derive(Message)]
#[rtype(result = "()")]
pub struct NewIndexedSnapshot {
    snapshot: Snapshot,
}

pub struct SnapshotStorage {
    db: Arc<DB>,
}

impl RocksDBObjectCRUD<Snapshot> for SnapshotStorage {
    fn db(&self) -> &DB {
        &self.db
    }

    fn prefix(&self) -> String {
        "bucket.snapshot:".into()
    }
}

impl SnapshotStorage {
    pub fn new(db: Arc<DB>) -> Self {
        Self { db }
    }

    // pub fn get_head() -> Option<Snapshot>;
}

impl Actor for SnapshotStorage {
    type Context = Context<Self>;
}

impl Handler<NewIndexedSnapshot> for SnapshotStorage {
    type Result = ();

    fn handle(&mut self, msg: NewIndexedSnapshot, ctx: &mut Self::Context) -> Self::Result {
        let refh = self.put_entity(&msg.snapshot);
    }
}
