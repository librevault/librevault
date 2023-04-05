use actix::{Actor, Addr, Context, Handler, SyncArbiter};
use actix_rt::System;

use crate::indexer::worker::IndexerWorker;
use crate::indexer::WrappedIndexingTask;

pub struct IndexerWorkerPool {
    arbiter: Addr<IndexerWorker>,
}

impl IndexerWorkerPool {
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

impl Actor for IndexerWorkerPool {
    type Context = Context<Self>;
}

impl Handler<WrappedIndexingTask> for IndexerWorkerPool {
    type Result = ();

    fn handle(&mut self, msg: WrappedIndexingTask, _ctx: &mut Self::Context) -> Self::Result {
        self.arbiter.do_send(msg);
    }
}
