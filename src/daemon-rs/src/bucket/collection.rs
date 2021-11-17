use std::collections::HashMap;
use std::sync::{Arc, RwLock};

use crate::bucket::Bucket;

struct BucketCollectionInner {
    buckets_byid: HashMap<Vec<u8>, Arc<Bucket>>,
}

pub struct BucketCollection {
    inner: RwLock<BucketCollectionInner>,
}

impl BucketCollection {
    pub fn new() -> BucketCollection {
        BucketCollection {
            inner: RwLock::new(BucketCollectionInner {
                buckets_byid: HashMap::new(),
            }),
        }
    }

    pub fn add_bucket(&mut self, bucket: Bucket) -> Arc<Bucket> {
        let bucket_arc = Arc::new(bucket);

        let mut inner_lock = self.inner.write().unwrap();
        let inner = &mut *inner_lock;

        inner
            .buckets_byid
            .insert(bucket_arc.get_id(), bucket_arc.clone());
        bucket_arc
    }

    pub fn get_bucket_byid(&self, bucket_id: &[u8]) -> Option<Arc<Bucket>> {
        let inner_lock = self.inner.read().unwrap();
        (*inner_lock).buckets_byid.get(bucket_id).cloned()
    }

    pub fn get_bucket_one(&self) -> Option<Arc<Bucket>> {
        let inner_lock = self.inner.read().unwrap();
        (*inner_lock)
            .buckets_byid
            .iter()
            .next()
            .map(|x| x.1)
            .cloned()
    }

    // pub fn drop_bucket(&mut self, bucket: Arc<Bucket>) -> Option<Arc<Bucket>> {
    //     self.buckets_byid.remove(bucket.get_id().as_slice())
    // }
}
