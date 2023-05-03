use std::path::PathBuf;

use actix::{Actor, Context, Handler, Message, MessageResponse};
use librevault_core::proto::ReferenceHash;

use crate::chunkstorage::materialized::MaterializedFolder;
use crate::chunkstorage::{ChunkStorageError, QueryResult};

#[derive(Message)]
#[rtype(result = "Response")]
pub enum Command {
    ResolveChunkMdByPlaintextHash { hash: ReferenceHash },
    ListUncommittedPaths,
}

#[derive(MessageResponse)]
pub enum Response {
    GetChunk(Result<ReferenceHash, ChunkStorageError>),
    ResolveChunk(Option<ReferenceHash>),
    ListDirtyPaths(Vec<PathBuf>),
}

impl Actor for MaterializedFolder {
    type Context = Context<Self>;
}

impl Handler<Command> for MaterializedFolder {
    type Result = Response;

    fn handle(&mut self, msg: Command, ctx: &mut Self::Context) -> Self::Result {
        match msg {
            Command::ResolveChunkMdByPlaintextHash { hash } => {
                // let (resp_tx, resp_rx) = tokio::sync::oneshot::channel();
                //
                // tokio::spawn(async move {
                //     let resp = self.resolve_chunk_md_by_plaintext_hash(&hash).await;
                //     resp_tx.send(resp).unwrap();
                // });
                // Response::ResolveChunk(resp_rx.blocking_recv().unwrap())
                Response::ResolveChunk(None)
            }
            Command::ListUncommittedPaths => Response::ListDirtyPaths(self.list_dirty_paths()),
        }
    }
}
