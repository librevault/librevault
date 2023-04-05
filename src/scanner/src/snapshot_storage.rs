use std::sync::Arc;

use librevault_core::indexer::reference::ReferenceMaker;
use librevault_core::proto::Snapshot;
use rocksdb::DB;

use crate::rdb::RocksDBObjectCRUD;

pub struct SnapshotStorage {
    db: Arc<DB>,
}

impl RocksDBObjectCRUD<Snapshot> for SnapshotStorage {
    fn db(&self) -> &DB {
        &*self.db
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
