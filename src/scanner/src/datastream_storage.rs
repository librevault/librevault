use std::sync::Arc;

use librevault_core::indexer::reference::ReferenceMaker;
use librevault_core::proto::{ChunkMetadata, DataStream, ReferenceHash};
use rocksdb::DB;

use crate::rdb::RocksDBObjectCRUD;

pub struct DataStreamStorage {
    db: Arc<DB>,
}

impl RocksDBObjectCRUD<DataStream> for DataStreamStorage {
    fn db(&self) -> &DB {
        &*self.db
    }

    fn prefix(&self) -> String {
        "bucket.datastream:".into()
    }
}

impl RocksDBObjectCRUD<ChunkMetadata> for DataStreamStorage {
    fn db(&self) -> &DB {
        &*self.db
    }

    fn prefix(&self) -> String {
        "bucket.chunk:".into()
    }
}

impl DataStreamStorage {
    pub fn new(db: Arc<DB>) -> Self {
        Self { db }
    }

    pub fn get_chunk_md_bulk(
        &self,
        hashes: &[ReferenceHash],
    ) -> Vec<(ReferenceHash, Option<ChunkMetadata>)> {
        self.get_entity_bulk(hashes)
    }

    pub fn get_chunk_md(&self, refh: &ReferenceHash) -> Option<ChunkMetadata> {
        self.get_entity(refh)
    }

    pub fn put_datastream(&self, datastream: &DataStream) {
        self.put_entity(datastream);
    }

    pub fn put_chunk_md(&self, chunk_md: &ChunkMetadata) {
        self.put_entity(chunk_md);
    }
}
