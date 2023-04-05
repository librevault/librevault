use std::path::{Path, PathBuf};

use actix::{Actor, ActorContext, AsyncContext, Context, Handler, Message, WeakAddr};
use notify::{RecommendedWatcher, RecursiveMode, Watcher};
use tracing::debug;

struct LocalDataChange {}

pub struct DirectoryWatcher {
    watcher: Option<RecommendedWatcher>,
    root: PathBuf,
}

#[derive(Message, Debug)]
#[rtype(result = "()")]
struct DirectoryWatcherEvent(Result<notify::Event, notify::Error>);

impl DirectoryWatcher {
    pub fn new(root: &Path) -> Self {
        Self {
            watcher: None,
            root: root.into(),
        }
    }
}

impl Actor for DirectoryWatcher {
    type Context = Context<Self>;

    fn started(&mut self, ctx: &mut Self::Context) {
        let addr = ctx.address().downgrade();
        let mut watcher = notify::recommended_watcher(move |ev| {
            if let Some(addr) = addr.upgrade() {
                addr.do_send(DirectoryWatcherEvent(ev));
            }
        })
        .unwrap();
        watcher.watch(&self.root, RecursiveMode::Recursive).unwrap();
        self.watcher = Some(watcher);
    }
}

impl Handler<DirectoryWatcherEvent> for DirectoryWatcher {
    type Result = ();

    fn handle(&mut self, msg: DirectoryWatcherEvent, ctx: &mut Self::Context) -> Self::Result {
        debug!("Change detected: {msg:?}")
    }
}
