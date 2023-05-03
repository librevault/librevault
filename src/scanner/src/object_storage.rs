use std::sync::Arc;

use actix::dev::{MessageResponse, OneshotSender};
use actix::{Actor, Context, Handler, Message};
use librevault_core::proto::{ObjectMetadata, ReferenceHash};
use rocksdb::DB;

use crate::rdb::RocksDBObjectCRUD;

pub struct ObjectMetadataStorage {
    db: Arc<DB>,
}

impl RocksDBObjectCRUD<ObjectMetadata> for ObjectMetadataStorage {
    fn db(&self) -> &DB {
        &*self.db
    }

    fn prefix(&self) -> String {
        "bucket.object:".into()
    }
}

impl ObjectMetadataStorage {
    pub fn new(db: Arc<DB>) -> Self {
        Self { db }
    }
}

#[derive(Message)]
#[rtype(result = "Response")]
pub enum Command {
    GetEntity { refh: ReferenceHash },
    PutEntity { entity: ObjectMetadata },
}

#[derive(MessageResponse)]
pub enum Response {
    Empty,
    GetEntity { entity: Option<ObjectMetadata> },
}

impl Actor for ObjectMetadataStorage {
    type Context = Context<Self>;
}

impl Handler<Command> for ObjectMetadataStorage {
    type Result = Response;

    fn handle(&mut self, msg: Command, ctx: &mut Self::Context) -> Self::Result {
        match msg {
            Command::GetEntity { refh } => Response::GetEntity {
                entity: self.get_entity(&refh),
            },
            Command::PutEntity { entity } => {
                self.put_entity(&entity);
                return Response::Empty;
            }
        }
    }
}
