use std::iter::zip;

use librevault_core::indexer::reference::{ReferenceExt, ReferenceMaker};
use librevault_core::proto::ReferenceHash;
use prost::Message;
use rocksdb::{IteratorMode, PrefixRange, ReadOptions, DB};

pub trait RocksDBObjectCRUD<T: Default + Message + ReferenceMaker> {
    fn db(&self) -> &DB;

    fn prefix(&self) -> String;

    fn key_for_entity(&self, refh: &ReferenceHash) -> String {
        self.prefix() + &refh.hex()
    }

    fn get_entity(&self, refh: &ReferenceHash) -> Option<T> {
        let obj = self.db().get(self.key_for_entity(refh)).unwrap()?;
        Some(T::decode(&*obj).unwrap())
    }

    fn get_entity_bulk(&self, refh: &[ReferenceHash]) -> Vec<(ReferenceHash, Option<T>)> {
        let key_iter = refh.iter().map(|hash| self.key_for_entity(hash));

        let all_chunks: Result<Vec<Option<Vec<u8>>>, rocksdb::Error> =
            self.db().multi_get(key_iter).into_iter().collect();

        let all_chunks = all_chunks.unwrap();

        let all_chunks: Vec<Option<T>> = all_chunks
            .into_iter()
            .map(|serialized_opt| serialized_opt.map(|serialized| T::decode(&*serialized).unwrap()))
            .collect();

        zip(refh.into_iter().cloned(), all_chunks).collect()
    }

    fn put_entity(&self, obj: &T) -> ReferenceHash {
        let (obj_s, obj_refh) = obj.reference();
        self.db()
            .put(self.key_for_entity(&obj_refh), &obj_s)
            .unwrap();
        obj_refh
    }

    fn get_all_entities(&self) -> Vec<T> {
        let mut opts = ReadOptions::default();
        opts.set_prefix_same_as_start(true);
        opts.set_iterate_range(PrefixRange(self.prefix()));
        let iter = self.db().iterator_opt(
            IteratorMode::From(self.prefix().as_bytes(), rocksdb::Direction::Forward),
            opts,
        );

        iter.map(|item| T::decode(&*item.unwrap().1).unwrap())
            .collect()
    }
}
