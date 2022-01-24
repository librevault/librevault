use crate::bucket::collection::BucketCollection;
use crate::bucket::{Bucket, BucketEvent};
use crate::settings::BucketConfig;
use log::trace;
use std::sync::{Arc, RwLock};
use tokio::sync::{broadcast, mpsc};

#[derive(Clone, Debug)]
pub enum BucketManagerEvent {
    Added(Arc<Bucket>),
    Removed(Arc<Bucket>),
    BucketEvent {
        bucket: Arc<Bucket>,
        event: BucketEvent,
    },
}

pub struct BucketManager {
    buckets: RwLock<BucketCollection>,
    event_tx: broadcast::Sender<BucketManagerEvent>,
}

impl BucketManager {
    pub fn new() -> Self {
        let (event_tx, _) = broadcast::channel(128);
        BucketManager {
            buckets: RwLock::new(BucketCollection::new()),
            event_tx,
        }
    }

    pub async fn add_bucket(&self, config: BucketConfig) {
        let mut lock = self.buckets.write().unwrap();

        let (bucket_event_tx, mut bucket_event_rx) = mpsc::channel(128);

        let bucket = Bucket::new(config, bucket_event_tx).await;
        let bucket_arc = (*lock).add_bucket(bucket);
        bucket_arc.initialize().await;
        trace!("Sending event for BucketAdded");

        let _ = self
            .event_tx
            .send(BucketManagerEvent::Added(bucket_arc.clone()));

        let event_tx_cloned = self.event_tx.clone();

        tokio::spawn(async move {
            loop {
                tokio::select! {
                    Some(event) = bucket_event_rx.recv() => {
                        let _ = event_tx_cloned.send(BucketManagerEvent::BucketEvent {bucket: bucket_arc.clone(), event});
                    },
                    else => break,
                }
            }
        });
    }

    pub fn get_event_channel(&self) -> broadcast::Receiver<BucketManagerEvent> {
        self.event_tx.subscribe()
    }

    pub fn get_bucket_byid(&self, bucket_id: &[u8]) -> Option<Arc<Bucket>> {
        self.buckets.read().unwrap().get_bucket_byid(bucket_id)
    }
}
