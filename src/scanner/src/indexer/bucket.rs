use std::collections::{BTreeMap, VecDeque};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;

use actix::{Actor, Addr, AsyncContext, Context, Handler};
use librevault_core::proto::ReferenceHash;
use slab::Slab;
use tracing::{error, warn};

use crate::chunkstorage::materialized::MaterializedFolder;
use crate::indexer::{
    GlobalIndexerActor, IndexingTask, IndexingTaskResult, IndexingTaskRoutingInfo,
    WrappedIndexingTask,
};

struct SnapshotTaskGroup {
    parent: Option<ReferenceHash>,
    // paths: HashSet<PathBuf>,
    results: BTreeMap<u64, IndexingTaskResult>,
    next_id: AtomicUsize,
}

impl SnapshotTaskGroup {
    fn new(parent: Option<ReferenceHash>) -> Self {
        Self {
            parent,
            results: Default::default(),
            next_id: AtomicUsize::new(1),
        }
    }

    fn is_finalized(&self) -> bool {
        false
    }

    fn next_id(&self) -> usize {
        self.next_id.fetch_add(1, Ordering::Relaxed)
    }

    fn set_result(&mut self, result: IndexingTaskResult) {
        error!("Got result: {result:?}");
        self.results.insert(result.routing_info.id as u64, result);
    }
}

pub struct BucketIndexer {
    global: Addr<GlobalIndexerActor>,

    groups: Slab<SnapshotTaskGroup>,
    groups_a: VecDeque<usize>,

    ms: Arc<MaterializedFolder>,
}

impl BucketIndexer {
    pub fn new(global: Addr<GlobalIndexerActor>, ms: Arc<MaterializedFolder>) -> Self {
        Self {
            global,
            groups: Default::default(),
            groups_a: Default::default(),
            ms,
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

        let wrapped = WrappedIndexingTask {
            routing_info: IndexingTaskRoutingInfo {
                group: group_id,
                id: group.next_id(),
                result_receiver: ctx.address().into(),
            },
            task,
            ms: self.ms.clone(),
        };
        self.global.do_send(wrapped);
    }
}

impl Handler<IndexingTaskResult> for BucketIndexer {
    type Result = ();

    fn handle(&mut self, msg: IndexingTaskResult, ctx: &mut Self::Context) -> Self::Result {
        let group_key = msg.routing_info.group;

        let mut group = &mut self.groups[group_key];
        group.set_result(msg);

        // Create snapshot
        if group.is_finalized() {
            let group = self.groups.remove(group_key);
            let mut objects = vec![];
            let mut chunks = vec![];
            let mut datastreams = vec![];

            for result in group.results.into_values() {
                objects.push(result.object);
                chunks.extend(result.chunks);
                datastreams.extend(result.datastreams);
            }

            // let snapshot = Snapshot {
            //     parents: group.parent.into(),
            //     r#type: snapshot::Type::Full as i32,
            //     object_md: objects.iter().map(|o| o.reference())
            // };
        }
    }
}
