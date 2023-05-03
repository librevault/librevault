use std::error::Error;
use std::os::unix::fs::FileExt;
use std::path::PathBuf;
use std::str::FromStr;

use librevault_core::indexer::reference::ReferenceExt;
use librevault_core::proto::ReferenceHash;
use prost::Message;
use serde::{Deserialize, Serialize};
use thiserror::Error as ThisError;

pub mod actor;
pub mod materialized;

#[derive(ThisError, Debug)]
pub enum ChunkStorageError {
    #[error("Chunk not found")]
    ChunkNotFound,
    #[error("Internal error: {0}")]
    InternalError(Box<dyn Error + Send>),
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
pub enum ChunkLocationHint {
    #[default]
    Empty,
    MaterializedLocation {
        path: PathBuf,
        offset: u64,
        length: usize,
    },
}

#[derive(Default)]
pub struct QueryResult {
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
