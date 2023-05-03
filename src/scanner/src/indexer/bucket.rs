use std::collections::VecDeque;
use std::sync::Arc;

use actix::{Actor, Addr, AsyncContext, Context, Handler, WeakRecipient};
use librevault_core::indexer::reference::ReferenceMaker;
use librevault_core::proto::{snapshot, ReferenceHash, Snapshot};
use slab::Slab;
use tracing::{debug, error, warn};

use crate::chunkstorage::materialized::MaterializedFolder;
use crate::datastream_storage::DataStreamStorage;
use crate::indexer::pool::IndexerWorkerPool;
use crate::indexer::{
    IndexingEvent, IndexingTask, IndexingTaskResult, IndexingTaskRoutingInfo, WrappedIndexingTask,
};

struct SnapshotTaskGroup {
    parent: Option<ReferenceHash>,
    // paths: HashSet<PathBuf>,
    tasks: Vec<(IndexingTask, Option<IndexingTaskResult>)>,

    total_objects: usize,
    total_chunks: usize,
    total_datastreams: usize,
}

impl SnapshotTaskGroup {
    fn new(parent: Option<ReferenceHash>) -> Self {
        Self {
            parent,

            tasks: vec![],

            total_objects: 0,
            total_chunks: 0,
            total_datastreams: 0,
        }
    }

    fn is_finalized(&self) -> bool {
        self.tasks.len() == self.total_objects
    }

    fn register_task(&mut self, task: &IndexingTask) -> usize {
        self.tasks.push((task.clone(), None));
        self.tasks.len() - 1
    }

    fn set_result(&mut self, result: IndexingTaskResult) {
        error!("Got result: {result:?}");
        self.total_objects += 1;
        self.total_chunks += result.chunks.len();
        self.total_datastreams += result.datastreams.len();

        let task_id = result.routing_info.id;
        debug_assert!(
            self.tasks[task_id].1.is_none(),
            "Indexing result is repeated!"
        );
        self.tasks[task_id].1 = Some(result);
    }
}

pub struct BucketIndexer {
    pool: Addr<IndexerWorkerPool>,

    groups: Slab<SnapshotTaskGroup>,
    groups_a: VecDeque<usize>,

    ms: Addr<MaterializedFolder>,
    dss: Arc<DataStreamStorage>,
    ev_handler: WeakRecipient<IndexingEvent>,
}

impl BucketIndexer {
    pub fn new(
        pool: Addr<IndexerWorkerPool>,
        ms: Addr<MaterializedFolder>,
        dss: Arc<DataStreamStorage>,
        ev_handler: WeakRecipient<IndexingEvent>,
    ) -> Self {
        Self {
            pool,
            groups: Default::default(),
            groups_a: Default::default(),
            ms,
            dss,
            ev_handler,
        }
    }
}

impl Actor for BucketIndexer {
    type Context = Context<Self>;
}

impl Handler<IndexingTask> for BucketIndexer {
    type Result = ();

    fn handle(&mut self, task: IndexingTask, ctx: &mut Self::Context) -> Self::Result {
        let group_id = if !self.groups_a.is_empty() {
            *self.groups_a.front().unwrap()
        } else {
            let group = SnapshotTaskGroup::new(None);
            let group_id = self.groups.insert(group);
            self.groups_a.push_back(group_id);
            group_id
        };
        warn!("Group ID: {group_id}");
        let group = &mut self.groups[group_id];

        let id = group.register_task(&task);

        let wrapped = WrappedIndexingTask {
            routing_info: IndexingTaskRoutingInfo {
                group: group_id,
                id,
                result_receiver: ctx.address().into(),
            },
            task,
            ms: self.ms.clone().downgrade(),
            dss: self.dss.clone(),
        };
        self.pool.do_send(wrapped);
    }
}

impl Handler<IndexingTaskResult> for BucketIndexer {
    type Result = ();

    fn handle(&mut self, msg: IndexingTaskResult, ctx: &mut Self::Context) -> Self::Result {
        let group_key = msg.routing_info.group;

        let group = &mut self.groups[group_key];
        group.set_result(msg);

        // Create snapshot
        if group.is_finalized() {
            let group = self.groups.remove(group_key);
            let mut objects = Vec::with_capacity(group.total_objects);
            let mut chunks = Vec::with_capacity(group.total_chunks);
            let mut datastreams = Vec::with_capacity(group.total_datastreams);

            for result in group.tasks.into_iter().map(|v| v.1.unwrap()) {
                objects.push(result.object);
                chunks.extend(result.chunks);
                datastreams.extend(result.datastreams);
            }

            debug!(
                "Building snapshot: objects={}, chunks={}, datastreams={}",
                objects.len(),
                chunks.len(),
                datastreams.len()
            );

            let snapshot = Snapshot {
                parents: vec![],
                r#type: snapshot::Type::Full as i32,
                object_md: objects.iter().map(|o| o.reference().1).collect(),
                datastream: datastreams.iter().map(|d| d.reference().1).collect(),
            };

            let ev_handler = self.ev_handler.upgrade().unwrap();
            ev_handler.do_send(IndexingEvent::SnapshotCreated {
                snapshot,
                objects,
                datastreams,
                chunks,
            });
        }
    }
}
