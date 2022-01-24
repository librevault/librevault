use std::fmt;
use std::fmt::{Debug, Formatter};
use std::path::{Path, PathBuf};
use std::sync::Arc;

use librevault_util::index::{Index, SignedMeta};
use librevault_util::secret::Secret;

mod collection;
pub mod manager;

use crate::settings::BucketConfig;
use librevault_util::enc_storage::EncryptedStorage;
use log::debug;
use tokio::sync::mpsc;

#[derive(Clone, Debug)]
pub enum BucketEvent {
    MetaAdded { signed_meta: SignedMeta },
}

pub struct Bucket {
    pub secret: Secret,
    root: PathBuf,

    pub index: Arc<Index>,
    chunk_storage: EncryptedStorage,

    event_tx: mpsc::Sender<BucketEvent>,
}

impl Bucket {
    async fn new(config: BucketConfig, event_tx: mpsc::Sender<BucketEvent>) -> Self {
        debug!("Creating bucket: {}", config.secret.get_id_hex());

        let system_dir = config.path.join(".librevault");

        tokio::fs::create_dir_all(&system_dir).await.unwrap();
        let index = Arc::new(Index::new(system_dir.join("index.db").as_path()));

        Bucket {
            secret: config.secret,
            root: config.path,
            index,
            chunk_storage: EncryptedStorage::new(&system_dir),
            event_tx,
        }
    }

    pub fn get_id(&self) -> Vec<u8> {
        self.secret.get_id()
    }

    pub fn get_id_hex(&self) -> String {
        self.secret.get_id_hex()
    }

    async fn initialize(&self) {
        let _ = tokio::task::block_in_place(move || self.index.migrate()); // TODO: block somewhere else

        // loop {
        //     let ev = watcher::async_watch(&self.root, RecursiveMode::Recursive).await;
        // }
    }

    pub(crate) async fn index_path(&self, path: &Path) {}
}

impl Debug for Bucket {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "(Bucket: id={}, loc={:?})", self.get_id_hex(), self.root)
    }
}
