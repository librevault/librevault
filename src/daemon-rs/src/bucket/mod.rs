use std::path::{Path, PathBuf};
use std::sync::{Arc, RwLock};

use collection::BucketCollection;
use librevault_util::index::Index;
use librevault_util::secret::Secret;

mod collection;
mod watcher;
use crate::settings::BucketConfig;
use hex::ToHex;
use librevault_util::indexer::{make_meta, sign_meta};
use log::debug;

pub struct Bucket {
    secret: Secret,
    root: PathBuf,

    index: Arc<Index>,
    system_dir: PathBuf,
}

impl Bucket {
    async fn new(config: BucketConfig) -> Self {
        let bucket_id_hex: String = config.secret.get_id().encode_hex();
        debug!("Creating bucket: {}", bucket_id_hex);

        let system_dir = config.path.join(".librevault");

        tokio::fs::create_dir_all(&system_dir).await.unwrap();
        let index = Arc::new(Index::new(system_dir.join("index.db").as_path()));

        Bucket {
            secret: config.secret,
            root: config.path,
            index,
            system_dir,
        }
    }

    fn get_id(&self) -> Vec<u8> {
        self.secret.get_id()
    }

    async fn launch_bucket(&self) {
        let _ = tokio::task::block_in_place(move || self.index.migrate()); // TODO: block somewhere else

        // loop {
        //     let ev = watcher::async_watch(&self.root, RecursiveMode::Recursive).await;
        // }
    }

    pub(crate) async fn index_path(&self, path: &Path) {
        let meta = make_meta(path, &self.root, &self.secret);
        if let Ok(meta) = meta {
            debug!("Meta: {:?}", &meta);
            let signed_meta = sign_meta(&meta, &self.secret);
            self.index.put_meta(signed_meta, true).unwrap();
        }
    }
}

pub struct BucketManager {
    buckets: Arc<RwLock<BucketCollection>>,
}

impl BucketManager {
    pub fn new() -> Self {
        BucketManager {
            buckets: Arc::new(RwLock::new(BucketCollection::new())),
        }
    }

    pub async fn add_bucket(&self, config: BucketConfig) {
        let mut lock = self.buckets.write().unwrap();

        let bucket = Bucket::new(config).await;
        let bucket_arc = (*lock).add_bucket(bucket);
        bucket_arc.launch_bucket().await;
    }

    pub fn get_bucket_byid(&self, bucket_id: &[u8]) -> Option<Arc<Bucket>> {
        self.buckets.read().unwrap().get_bucket_byid(bucket_id)
    }
}
