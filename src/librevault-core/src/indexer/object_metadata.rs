use crate::indexer::object_type::ObjectTypeIndexer;
use crate::indexer::reference::ReferenceMaker;
use crate::indexer::IndexingError;
use crate::path::PointerPath;
use crate::proto::object_metadata::FileTimes;
use crate::proto::{object_metadata::ObjectType, ObjectMetadata};
use std::fs;
#[cfg(target_family = "unix")]
use std::os::unix::fs::MetadataExt;
use std::path::Path;
use std::time::SystemTime;
use tracing::debug;

impl ReferenceMaker for ObjectMetadata {}

pub trait FileTimesIndexer: Sized {
    fn from_metadata(metadata: &fs::Metadata) -> Result<Self, IndexingError>;
}

impl FileTimesIndexer for FileTimes {
    fn from_metadata(metadata: &fs::Metadata) -> Result<Self, IndexingError> {
        let [mtime, atime, crtime] = [
            metadata.modified()?,
            metadata.accessed()?,
            metadata.created()?,
        ]
        .map(|t| {
            let t = t.duration_since(SystemTime::UNIX_EPOCH).unwrap();
            t.as_nanos() as i64
        });

        Ok(Self {
            mtime,
            atime,
            crtime,
        })
    }
}

pub trait ObjectMetadataIndexer: Sized {
    fn from_path(path: &Path, root: &Path, preserve_symlinks: bool) -> Result<Self, IndexingError>;
}

impl ObjectMetadataIndexer for ObjectMetadata {
    fn from_path(path: &Path, root: &Path, preserve_symlinks: bool) -> Result<Self, IndexingError> {
        let mut res = Self::default();

        let metadata = match preserve_symlinks {
            true => fs::symlink_metadata(path)?,
            false => fs::metadata(path)?,
        };

        res.path = PointerPath::from_path(path, root, true)?.into_vec();

        res.r#type = ObjectType::from_metadata(&metadata)? as i32;
        res.filetimes = FileTimes::from_metadata(&metadata).ok();

        #[cfg(target_family = "unix")]
        {
            res.mode = Some(metadata.mode());
            res.uid = Some(metadata.uid());
            res.gid = Some(metadata.gid());
        }

        #[cfg(target_family = "windows")]
        {
            res.file_attributes = Some(metadata.file_attributes());
        }

        if res.r#type == ObjectType::Symlink as i32 {
            res.symlink_target = Some(
                fs::read_link(path)?
                    .into_os_string()
                    .into_string()
                    .map_err(|_| IndexingError::UnicodeError)?,
            );
        }

        debug!("Got ObjectMeta for path={path:?} root={root:?} objectmeta={res:?}");

        Ok(res)
    }
}
