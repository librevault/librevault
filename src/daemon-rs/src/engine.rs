use crate::engine::BucketCommand::MakeSnapshot;
use crate::watcher::WatcherWrapper;
use crate::BucketConfig;
use async_trait::async_trait;
use librevault_core::index::Index;
use librevault_core::indexer::{make_full_snapshot, sign_revision};
use librevault_util::secret::Secret;
use notify::RecursiveMode::Recursive;
use notify::{DebouncedEvent, Watcher};
use prost::Message as ProstMessage;
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use tiny_tokio_actor::{
    Actor, ActorContext, ActorRef, ActorSystem, EventBus, Handler, Message as ActorMessage,
    SystemEvent,
};
use tokio::sync::mpsc;
use tokio::sync::mpsc::UnboundedReceiver;
use tracing::{debug, error, info, instrument, trace, warn};

pub struct BucketActor {
    secret: Secret,
    pub path: PathBuf,

    index: Index,
}

#[derive(Debug, Clone)]
pub enum BucketCommand {
    MakeSnapshot,
}

impl ActorMessage for BucketCommand {
    type Response = ();
}

#[derive(Clone, Debug)]
struct BucketEvent;

impl SystemEvent for BucketEvent {}

impl BucketActor {
    #[instrument]
    pub(crate) fn new(config: BucketConfig) -> BucketActor {
        debug!("Creating Bucket {:?}", config);
        let index = Index::init(Index::default_index_file(config.path.as_path()).as_path());

        BucketActor {
            secret: config.secret,
            path: config.path,
            index,
        }
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
}

impl Actor<BucketEvent> for BucketActor {}

#[async_trait]
impl Handler<BucketEvent, BucketCommand> for BucketActor {
    async fn handle(&mut self, msg: BucketCommand, ctx: &mut ActorContext<BucketEvent>) -> () {
        tokio::task::block_in_place(|| self.make_snapshot()).await;
    }
}

struct BucketEntry {
    bucket_config: BucketConfig,
    actor_ref: ActorRef<BucketEvent, BucketActor>,
}

pub struct Engine {
    watcher_wrapper: WatcherWrapper,
    watcher_rx: UnboundedReceiver<notify::DebouncedEvent>,

    buckets: HashMap<Vec<u8>, BucketEntry>,

    system: ActorSystem<BucketEvent>,
}

impl Engine {
    pub(crate) fn new() -> Engine {
        let (watcher_tx, watcher_rx) = mpsc::unbounded_channel();
        let watcher_wrapper = WatcherWrapper::new(move |event| {
            watcher_tx.send(event);
        });

        let bus = EventBus::new(1000);
        let system = ActorSystem::new("buckets", bus);

        Self {
            watcher_wrapper,
            watcher_rx,

            buckets: HashMap::default(),
            system,
        }
    }

    pub async fn add_bucket(&mut self, bucket_config: BucketConfig) {
        let bucket_actor = BucketActor::new(bucket_config.clone());

        let bucket_id = bucket_actor.bucket_id();

        let actor_ref = self
            .system
            .create_actor("bucket", bucket_actor)
            .await
            .unwrap();

        self.buckets.insert(
            bucket_id,
            BucketEntry {
                bucket_config: bucket_config.clone(),
                actor_ref,
            },
        );

        self.watcher_wrapper
            .watcher
            .watch(bucket_config.path, Recursive)
            .expect("Cannot set up watch for bucket");
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
                            entry.actor_ref.ask(MakeSnapshot).await.unwrap();
                        }else{
                            warn!("Path {path:?} is not assigned to bucket.");
                        }
                    }
                }
            }
        }
    }
}
