use std::path::{Path, PathBuf};

use actix::{Actor, ActorContext, AsyncContext, Handler, Message};
use notify::{RecommendedWatcher, Watcher};

pub mod actor;

pub struct DirectoryWatcher {
    watcher: Box<dyn Watcher + Send + Sync + 'static>,
}

impl DirectoryWatcher {
    pub fn new(watcher: Box<dyn Watcher + Send + Sync + 'static>) -> Self {
        Self {
            watcher,
        }
    }
}
