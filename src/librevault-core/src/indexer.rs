use crate::encryption::{encrypt, generate_rand_buf};
use crate::proto::meta;
use crate::proto::meta::{
    revision::EncryptedPart as EncryptedRevision, revision::Kind as RevisionKind, ChunkMeta,
    EncryptionAlgorithm, EncryptionMetadata, ObjectKind, ObjectMeta, Revision, SignedRevision,
};

use librevault_util::path_normalize::{normalize, NormalizationError};
use librevault_util::secret::Secret;
use multihash::{Code, MultihashDigest};
use prost::Message;
use rabin::{Rabin, RabinParams};
use std::fs;
use std::io;
use std::io::{BufReader, ErrorKind, Read};
use std::path::Path;
use tracing::{debug, trace};
use walkdir::WalkDir;

#[derive(Debug)]
pub enum IndexingError {
    IoError(io::Error),
    UnsupportedType,
    NormalizationError(NormalizationError),
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

fn make_chunk(data: &[u8], secret: &Secret) -> ChunkMeta {
    let symmetric_key = secret.get_symmetric_key().unwrap();

    let encryption_metadata = EncryptionMetadata {
        algorithm: EncryptionAlgorithm::AesGcmSiv as i32,
        iv: generate_rand_buf(12),
    };

    let encrypted_data = encrypt(data, symmetric_key, encryption_metadata);
    let ciphertext_hash = Code::Sha3_256.digest(&*encrypted_data.ciphertext);

    debug!("New chunk size: {}, hash: {ciphertext_hash:?}", data.len());

    ChunkMeta {
        ciphertext_hash: ciphertext_hash.to_bytes(),
        encryption_metadata: encrypted_data.metadata,
        size: data.len() as u32,
    }
}

fn make_chunks_for_buf<R: Read>(
    reader: R,
    secret: &Secret,
) -> Result<Vec<ChunkMeta>, IndexingError> {
    Rabin::new(reader, RabinParams::default())
        .map(|chunk| Ok(make_chunk(chunk?.as_slice(), secret)))
        .collect()
}

fn make_chunks(path: &Path, secret: &Secret) -> Result<Vec<ChunkMeta>, IndexingError> {
    trace!("Trying to make chunks for: {:?}", path);

    let f = fs::File::open(path)?;
    let reader = BufReader::new(f);
    let chunks = make_chunks_for_buf(reader, secret)?;

    trace!("Total chunks for path {:?}: {}", path, chunks.len());

    Ok(chunks)
}

fn make_datastream(chunks: &[ChunkMeta]) -> meta::DataStream {
    meta::DataStream {
        chunk_ids: chunks
            .iter()
            .map(|chunk| chunk.ciphertext_hash.clone())
            .collect(),
    }
}

#[tracing::instrument]
fn make_objectmeta(
    path: &Path,
    root: &Path,
    secret: &Secret,
) -> Result<(ObjectMeta, Vec<ChunkMeta>), IndexingError> {
    debug!(
        "Requested ObjectMeta indexing: path={:?} root={:?}",
        path, root
    );

    let mut objectmeta = ObjectMeta::default();

    let objectmeta_kind = make_kind(path, true)?;

    objectmeta.kind = objectmeta_kind as i32;
    objectmeta.name = normalize(path, root, true)?;

    let mut chunks = vec![];

    if objectmeta_kind == ObjectKind::File {
        chunks = make_chunks(path, secret)?;
        if !chunks.is_empty() {
            objectmeta
                .data_streams
                .insert(String::from(""), make_datastream(chunks.as_slice()));
        }
    }

    debug!(
        "Got ObjectMeta for path={:?} root={:?} objectmeta={:?} chunks={:?}",
        path, root, objectmeta, chunks
    );

    Ok((objectmeta, chunks))
}

#[tracing::instrument]
pub fn make_revision(paths: &[&Path], root: &Path, secret: &Secret) -> meta::Revision {
    let mut chunks = vec![];
    let mut objectmetas = vec![];

    for path in paths {
        let result = make_objectmeta(path, root, secret);
        if let Ok((om_objectmeta, om_chunks)) = result {
            chunks.extend_from_slice(&*om_chunks);
            objectmetas.push(om_objectmeta);
        }
    }

    let mut revision_encrypted = EncryptedRevision::default();
    revision_encrypted.objects = objectmetas;

    let encryption_metadata = EncryptionMetadata {
        algorithm: EncryptionAlgorithm::AesGcmSiv as i32,
        iv: generate_rand_buf(12),
    };

    let mut revision = Revision::default();
    revision.kind = RevisionKind::Snapshot as i32;
    revision.chunks = chunks;
    revision.encrypted = Some(encrypt(
        &revision_encrypted.encode_to_vec(),
        secret.get_symmetric_key().unwrap(),
        encryption_metadata,
    ));
    revision
}

pub fn make_full_snapshot(root: &Path, secret: &Secret) -> Revision {
    let mut entries = vec![];

    for entry in WalkDir::new(root).min_depth(1) {
        debug!("{:?}", entry);
        entries.push(entry.unwrap().into_path())
    }

    let paths: Vec<&Path> = entries.iter().map(|f| f.as_path()).collect();

    make_revision(paths.as_slice(), root, secret)
}

pub fn sign_revision(revision: &meta::Revision, secret: &Secret) -> SignedRevision {
    let revision = revision.encode_to_vec();
    let signature = secret.sign(&revision).unwrap();

    SignedRevision {
        revision,
        signature,
    }
}

fn make_kind(path: &Path, preserve_symlinks: bool) -> Result<ObjectKind, IndexingError> {
    let metadata = match preserve_symlinks {
        true => fs::symlink_metadata(path),
        false => fs::metadata(path),
    };

    if let Err(e) = metadata {
        return match e.kind() {
            ErrorKind::NotFound => Ok(ObjectKind::Tombstone),
            _ => Err(IndexingError::from(e)),
        };
    }

    let file_type = metadata.unwrap().file_type();
    if file_type.is_file() {
        Ok(ObjectKind::File)
    } else if file_type.is_dir() {
        Ok(ObjectKind::Directory)
    // } else if file_type.is_symlink() {
    //     Ok(ObjectKind::Symlink)
    } else {
        Err(IndexingError::UnsupportedType)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_make_kind() {
        let temp = tempfile::tempdir().unwrap();
        assert_eq!(make_kind(temp.path(), true).unwrap(), ObjectKind::Directory);
        let tempf = tempfile::NamedTempFile::new().unwrap();
        assert_eq!(make_kind(tempf.path(), true).unwrap(), ObjectKind::File);
    }
}
