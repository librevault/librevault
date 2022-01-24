use crate::proto::meta::{ObjectMeta, Revision};

pub trait RevisionExt {
    fn chunks_size(&self) -> u64;
}

impl RevisionExt for Revision {
    fn chunks_size(&self) -> u64 {
        self.chunks.iter().map(|chunk| chunk.size as u64).sum()
    }
}

pub trait ObjectMetaExt {
    fn chunks_count(&self) -> usize;
}

impl ObjectMetaExt for ObjectMeta {
    fn chunks_count(&self) -> usize {
        self.data_streams.iter().map(|s| s.1.chunk_ids.len()).sum()
    }
}
