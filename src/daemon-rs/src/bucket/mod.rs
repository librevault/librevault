use std::borrow::Cow;
use std::fmt;
use std::fmt::Formatter;
use std::path::{Path, PathBuf};
use std::sync::Arc;

use librevault_util::index::{Index, SignedMeta};
use librevault_util::secret::Secret;

mod collection;
pub mod manager;

use crate::settings::BucketConfig;
// use librevault_util::index::proto::Meta;
use librevault_util::indexer::{make_meta, sign_meta};
use log::debug;
use tokio::sync::mpsc;

#[derive(Clone, Debug)]
pub enum BucketEvent {
    MetaAdded { signed_meta: SignedMeta },
}

pub struct Bucket {
    secret: Secret,
    root: PathBuf,

    index: Arc<Index>,

    event_sender: mpsc::Sender<BucketEvent>,
}

impl Bucket {
    async fn new(config: BucketConfig, event_sender: mpsc::Sender<BucketEvent>) -> Self {
        debug!("Creating bucket: {}", config.secret.get_id_hex());

        let system_dir = config.path.join(".librevault");

        tokio::fs::create_dir_all(&system_dir).await.unwrap();
        let index = Arc::new(Index::new(system_dir.join("index.db").as_path()));

        Bucket {
            secret: config.secret,
            root: config.path,
            index,
            event_sender,
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

    pub(crate) async fn index_path(&self, path: &Path) {
        let meta = make_meta(path, &self.root, &self.secret);
        if let Ok(meta) = meta {
            debug!("Meta: {:?}", &meta);
            let signed_meta = sign_meta(&meta, &self.secret);
            self.index.put_meta(&signed_meta, true).unwrap();

            self.event_sender
                .send(BucketEvent::MetaAdded { signed_meta })
                .await
                .expect("Channel must be open");
        }
    }
}

impl fmt::Debug for Bucket {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "(Bucket: id={}, loc={:?})", self.get_id_hex(), self.root)
    }
}
