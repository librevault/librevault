use std::io;

use crate::path::NormalizationError;

pub mod chunk_metadata;
pub mod datastream;
pub mod object_metadata;
pub mod object_type;
pub mod reference;
pub mod snapshot;

#[derive(Debug)]
pub enum IndexingError {
    IoError(io::Error),
    UnsupportedType,
    NormalizationError(NormalizationError),
    UnicodeError,
}

impl From<io::Error> for IndexingError {
    fn from(error: io::Error) -> Self {
        IndexingError::IoError(error)
    }
}

impl From<NormalizationError> for IndexingError {
    fn from(error: NormalizationError) -> Self {
        IndexingError::NormalizationError(error)
    }
}
