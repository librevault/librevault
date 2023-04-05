use std::path::PathBuf;
use std::sync::Arc;

use actix::{Actor, Addr, AsyncContext, Context, Handler, Message, SyncArbiter, WeakRecipient};
use actix_rt::System;
use librevault_core::path::PointerPath;
use librevault_core::proto::{ChunkMetadata, DataStream, ObjectMetadata, ReferenceHash};
use worker::IndexerWorker;

use crate::chunkstorage::materialized::MaterializedFolder;

pub mod bucket;
pub mod worker;

pub enum Policy {
    Orphan,
    SnapshotParent(ReferenceHash),
}

#[derive(Message)]
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

pub struct GlobalIndexerActor {
    arbiter: Addr<IndexerWorker>,
}

impl GlobalIndexerActor {
    pub fn new() -> Self {
        let threads = std::thread::available_parallelism()
            .map(|n| n.get())
            .unwrap_or(1);
        let arbiter = SyncArbiter::start(threads, || {
            IndexerWorker::new(System::current().arbiter().clone())
        });
        Self { arbiter }
    }
}

impl Actor for GlobalIndexerActor {
    type Context = Context<Self>;
}

impl Handler<WrappedIndexingTask> for GlobalIndexerActor {
    type Result = ();

    fn handle(&mut self, msg: WrappedIndexingTask, _ctx: &mut Self::Context) -> Self::Result {
        self.arbiter.do_send(msg);
    }
}
