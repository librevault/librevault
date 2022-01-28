use crate::engine::BucketCommand::MakeSnapshot;
use crate::watcher::WatcherWrapper;
use crate::BucketConfig;
use librevault_core::index::Index;
use librevault_core::indexer::{make_full_snapshot, sign_revision};
use librevault_util::secret::Secret;
use notify::RecursiveMode::Recursive;
use notify::{DebouncedEvent, Watcher};
use prost::Message;
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use tokio::sync::mpsc;
use tokio::sync::mpsc::UnboundedReceiver;
use tracing::{debug, error, info, instrument, trace, warn};

pub struct BucketState {
    secret: Secret,
    pub path: PathBuf,

    index: Index,

    event_rx: mpsc::UnboundedReceiver<BucketCommand>,
}

#[derive(Debug)]
pub enum BucketCommand {
    MakeSnapshot,
}
// pub enum BucketEvent {}

type Sender = mpsc::UnboundedSender<BucketCommand>;
type Receiver = mpsc::UnboundedReceiver<BucketCommand>;

impl BucketState {
    #[instrument]
    pub(crate) fn new(
        config: BucketConfig,
        // cmd_tx: mpsc::UnboundedSender<BucketCommand>,
    ) -> (BucketState, Sender) {
        debug!("Creating Bucket {:?}", config);
        let index = Index::init(Index::default_index_file(config.path.as_path()).as_path());

        let (event_tx, event_rx) = mpsc::unbounded_channel();

        (
            BucketState {
                secret: config.secret,
                path: config.path,
                index,
                event_rx,
            },
            event_tx,
        )
    }

    pub fn bucket_id(&self) -> Vec<u8> {
        self.secret.get_id()
    }

    pub async fn make_snapshot(&self) {
        let rev = make_full_snapshot(&self.path.as_path(), &self.secret);
        let rev_signed = sign_revision(&rev, &self.secret);
        let rev_size: Vec<u8> = rev.encode_to_vec();
        info!("Revision size: {}", rev_size.len());
        trace!("{rev:?}");
    }

    pub async fn start(&mut self) {
        loop {
            tokio::select! {
                Some(event) = self.event_rx.recv() => {
                    debug!("{event:?}");
                    match event {
                        MakeSnapshot => {
                            tokio::task::block_in_place(|| self.make_snapshot()).await;
                        }
                    }
                }
            }
        }
    }
}

struct BucketEntry {
    bucket_config: BucketConfig,
    cmd_tx: Sender,
}

pub struct Engine {
    watcher_wrapper: WatcherWrapper,
    watcher_rx: UnboundedReceiver<notify::DebouncedEvent>,

    buckets: HashMap<Vec<u8>, BucketEntry>,
}

impl Engine {
    pub(crate) fn new() -> Engine {
        let (watcher_tx, watcher_rx) = mpsc::unbounded_channel();
        let watcher_wrapper = WatcherWrapper::new(move |event| {
            watcher_tx.send(event);
        });

        Self {
            watcher_wrapper,
            watcher_rx,

            buckets: HashMap::default(),
        }
    }

    pub async fn add_bucket(&mut self, bucket_config: BucketConfig) {
        let (mut bucket_state, cmd_tx) = BucketState::new(bucket_config.clone());

        let bucket_id = bucket_state.bucket_id();

        self.buckets.insert(
            bucket_id,
            BucketEntry {
                bucket_config: bucket_config.clone(),
                cmd_tx,
            },
        );

        self.watcher_wrapper
            .watcher
            .watch(bucket_config.path, Recursive)
            .expect("Cannot set up watch for bucket");

        tokio::spawn(async move { bucket_state.start().await });
    }

    fn get_entry_for_path(&self, path: &Path) -> Option<&BucketEntry> {
        self.buckets
            .iter()
            .map(|entry| {
                trace!("{:?}", entry.1.bucket_config.path);
                entry
            })
            .find(|entry| path.starts_with(&entry.1.bucket_config.path))
            .map(|entry| entry.1)
    }

    pub async fn start(&mut self) {
        loop {
            tokio::select! {
                Some(event) = self.watcher_rx.recv() => {
                    debug!("{event:?}");
                    let ev_path = match event {
                        DebouncedEvent::Create(path) => Some(path),
                        DebouncedEvent::Remove(path) => Some(path),
                        DebouncedEvent::Write(path) => Some(path),
                        DebouncedEvent::Chmod(path) => Some(path),
                        DebouncedEvent::Rename(path1, _) => Some(path1),
                        _ => None
                    };

                    if let Some(path) = ev_path {
                        if let Some(entry) = self.get_entry_for_path(&path) {
                            entry.cmd_tx.send(MakeSnapshot);
                        }else{
                            warn!("Path {path:?} is not assigned to bucket.");
                        }
                    }
                }
            }
        }
    }
}
