use std::path::PathBuf;

use actix::Message;
use async_trait::async_trait;
use notify::{RecursiveMode, Watcher};
use ractor::{cast, Actor as RactorActor, ActorProcessingErr, ActorRef};
use tracing::debug;

use crate::directory_watcher::DirectoryWatcher;

#[derive(Message, Debug)]
#[rtype(result = "()")]
pub struct Command(Result<notify::Event, notify::Error>);

pub struct DirectoryWatcherActor;
pub struct Arguments {
    pub(crate) root: PathBuf,
}

#[async_trait]
impl RactorActor for DirectoryWatcherActor {
    type Msg = Command;
    type State = DirectoryWatcher;
    type Arguments = Arguments;

    async fn pre_start(
        &self,
        myself: ActorRef<Self::Msg>,
        args: Self::Arguments,
    ) -> Result<Self::State, ActorProcessingErr> {
        let mut watcher = notify::recommended_watcher(move |ev| {
            cast!(myself, Command(ev)).unwrap();
        })
        .unwrap();
        watcher.watch(&args.root, RecursiveMode::Recursive).unwrap();

        let state = DirectoryWatcher::new(Box::new(watcher));

        Ok(state)
    }

    async fn handle(
        &self,
        myself: ActorRef<Self::Msg>,
        message: Self::Msg,
        state: &mut Self::State,
    ) -> Result<(), ActorProcessingErr> {
        debug!("Change detected: {message:?}");
        Ok(())
    }
}
