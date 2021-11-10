use crate::bucket::collection::BucketCollection;
use crate::bucket::{Bucket, BucketEvent};
use crate::settings::BucketConfig;
use log::trace;
use std::sync::{Arc, RwLock};
use tokio::sync::{broadcast, mpsc};

#[derive(Clone, Debug)]
pub enum BucketManagerEvent {
    BucketAdded(Arc<Bucket>),
    BucketRemoved(Arc<Bucket>),
    BucketEvent {
        bucket: Arc<Bucket>,
        event: BucketEvent,
    },
}

pub struct BucketManager {
    buckets: Arc<RwLock<BucketCollection>>,
    event_sender: broadcast::Sender<BucketManagerEvent>,
}

impl BucketManager {
    pub fn new() -> Self {
        let (event_sender, _) = broadcast::channel(128);
        BucketManager {
            buckets: Arc::new(RwLock::new(BucketCollection::new())),
            event_sender,
        }
    }

    pub async fn add_bucket(&self, config: BucketConfig) {
        let mut lock = self.buckets.write().unwrap();

        let (event_sender_bucket, mut event_receiver_bucket) = mpsc::channel(128);

        let bucket = Bucket::new(config, event_sender_bucket).await;
        let bucket_arc = (*lock).add_bucket(bucket);
        bucket_arc.initialize().await;
        trace!("Sending event for BucketAdded");

        let _ = self
            .event_sender
            .send(BucketManagerEvent::BucketAdded(bucket_arc.clone()));

        let event_sender_local = self.event_sender.clone();

        tokio::spawn(async move {
            loop {
                tokio::select! {
                    event = event_receiver_bucket.recv() => {
                        if let Some(event) = event {
                            event_sender_local.send(BucketManagerEvent::BucketEvent {bucket: bucket_arc.clone(), event});
                        } else { break; }
                    }
                }
            }
        });
    }

    pub fn get_event_channel(&self) -> broadcast::Receiver<BucketManagerEvent> {
        self.event_sender.subscribe()
    }

    pub fn get_bucket_byid(&self, bucket_id: &[u8]) -> Option<Arc<Bucket>> {
        self.buckets.read().unwrap().get_bucket_byid(bucket_id)
    }
}
