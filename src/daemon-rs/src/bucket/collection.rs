use std::collections::HashMap;
use std::sync::Arc;

use crate::bucket::Bucket;

pub struct BucketCollection {
    buckets_byid: HashMap<Vec<u8>, Arc<Bucket>>,
}

impl BucketCollection {
    pub fn new() -> BucketCollection {
        BucketCollection {
            buckets_byid: HashMap::new(),
        }
    }

    pub fn add_bucket(&mut self, bucket: Bucket) -> Arc<Bucket> {
        let bucket_arc = Arc::new(bucket);

        self.buckets_byid.insert(bucket_arc.get_id(), bucket_arc.clone());
        bucket_arc
    }

    // pub fn drop_bucket(&mut self, bucket: Arc<Bucket>) -> Option<Arc<Bucket>> {
    //     self.buckets_byid.remove(bucket.get_id().as_slice())
    // }
}
