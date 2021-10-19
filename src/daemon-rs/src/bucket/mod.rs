use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::{Arc, RwLock};
use std::time::Duration;

use futures::{
    channel::mpsc::{channel, Receiver},
    SinkExt, StreamExt,
};
use notify::{Event, RecommendedWatcher, RecursiveMode, Watcher};
use tokio::sync::watch;

use collection::BucketCollection;
use librevault_util::index::Index;
use librevault_util::secret::OpaqueSecret;

mod collection;
mod watcher;

pub struct Bucket {
    secret: OpaqueSecret,
    root: PathBuf,

    index: Index,
    system_dir: PathBuf,
}

impl Bucket {
    async fn new(config: BucketConfig) -> Self {
        let system_dir = config.path.join(".librevault");

        tokio::fs::create_dir_all(&system_dir).await.unwrap();
        let index = Index::new(system_dir.join("index.db").as_path());

        Bucket { secret: config.secret, root: config.path, index, system_dir }
    }

    fn get_id(&self) -> Vec<u8> {
        self.secret.get_id()
    }

    async fn launch_bucket(&self) {
        tokio::task::block_in_place(move || {
            self.index.migrate();
        });

        // loop {
        //     let ev = watcher::async_watch(&self.root, RecursiveMode::Recursive).await;
        // }
    }
}

pub struct BucketManager {
    buckets: Arc<RwLock<BucketCollection>>,
}

pub struct BucketConfig {
    pub secret: OpaqueSecret,
    pub path: PathBuf,
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
}
