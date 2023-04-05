use crate::indexer::IndexingError;
use crate::proto::object_metadata::ObjectType;
use std::fs::Metadata;

pub trait ObjectTypeIndexer: Sized {
    fn from_metadata(metadata: &Metadata) -> Result<Self, IndexingError>;
}

impl ObjectTypeIndexer for ObjectType {
    fn from_metadata(metadata: &Metadata) -> Result<Self, IndexingError> {
        let file_type = metadata.file_type();
        if file_type.is_file() {
            Ok(ObjectType::File)
        } else if file_type.is_dir() {
            Ok(ObjectType::Directory)
        } else if file_type.is_symlink() {
            Ok(ObjectType::Symlink)
        } else {
            Err(IndexingError::UnsupportedType)
        }
    }
}
