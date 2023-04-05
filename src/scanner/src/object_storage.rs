use std::sync::Arc;

use librevault_core::proto::ObjectMetadata;
use rocksdb::DB;

use crate::rdb::RocksDBObjectCRUD;

pub struct ObjectMetadataStorage {
    db: Arc<DB>,
}

impl RocksDBObjectCRUD<ObjectMetadata> for ObjectMetadataStorage {
    fn db(&self) -> &DB {
        &*self.db
    }

    fn prefix(&self) -> String {
        "bucket.object:".into()
    }
}

impl ObjectMetadataStorage {
    pub fn new(db: Arc<DB>) -> Self {
        Self { db }
    }
}
