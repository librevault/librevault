use std::path::PathBuf;
use std::sync::Arc;

use actix::{Message, WeakRecipient};
use librevault_core::proto::{ChunkMetadata, DataStream, ObjectMetadata, ReferenceHash, Snapshot};

use crate::chunkstorage::materialized::MaterializedFolder;

pub mod bucket;
pub mod pool;
pub mod worker;

#[derive(Clone)]
pub enum Policy {
    Orphan,
    SnapshotParent(ReferenceHash),
}

#[derive(Message, Clone)]
#[rtype(result = "()")]
pub struct IndexingTask {
    pub policy: Policy,
    pub path: PathBuf,
    pub root: PathBuf,
}

#[derive(Debug)]
struct IndexingTaskRoutingInfo {
    group: usize,
    id: usize,
    result_receiver: WeakRecipient<IndexingTaskResult>,
}

#[derive(Message)]
#[rtype(result = "()")]
struct WrappedIndexingTask {
    routing_info: IndexingTaskRoutingInfo,
    task: IndexingTask,

    ms: Arc<MaterializedFolder>,
}

#[derive(Message, Debug)]
#[rtype(result = "()")]
struct IndexingTaskResult {
    routing_info: IndexingTaskRoutingInfo,

    object: ObjectMetadata,
    chunks: Vec<ChunkMetadata>,
    datastreams: Vec<DataStream>,
}

#[derive(Message, Debug)]
#[rtype(result = "()")]
pub enum IndexingEvent {
    SnapshotCreated {
        snapshot: Snapshot,
        objects: Vec<ObjectMetadata>,
        datastreams: Vec<DataStream>,
        chunks: Vec<ChunkMetadata>,
    },
}
