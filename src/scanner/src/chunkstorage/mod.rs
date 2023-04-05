use std::error::Error;
use std::os::unix::fs::FileExt;
use std::path::PathBuf;
use std::str::FromStr;

use librevault_core::indexer::reference::ReferenceExt;
use librevault_core::proto::ReferenceHash;
use prost::Message;
use serde::{Deserialize, Serialize};
use thiserror::Error as ThisError;

pub mod materialized;

#[derive(ThisError, Debug)]
enum ChunkStorageError {
    #[error("Chunk not found")]
    ChunkNotFound,
    #[error("Internal error: {0}")]
    InternalError(Box<dyn Error>),
}

impl From<rocksdb::Error> for ChunkStorageError {
    fn from(value: rocksdb::Error) -> Self {
        Self::InternalError(Box::new(value))
    }
}

impl From<prost::DecodeError> for ChunkStorageError {
    fn from(value: prost::DecodeError) -> Self {
        Self::InternalError(Box::new(value))
    }
}

#[derive(Default)]
enum ChunkLocationHint {
    #[default]
    Empty,
    MaterializedLocation {
        path: PathBuf,
        offset: u64,
        length: usize,
    },
}

#[derive(Default)]
struct QueryResult {
    chunk: Vec<u8>,
    hint: ChunkLocationHint,
}

#[async_trait::async_trait]
pub trait ChunkProvider {
    async fn get(
        &self,
        hash: &ReferenceHash,
        hint: Option<ChunkLocationHint>,
    ) -> Result<QueryResult, ChunkStorageError>;
}
