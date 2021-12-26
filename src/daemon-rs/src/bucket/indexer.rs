use librevault_util::aescbc::{encrypt_aes256, encrypt_chunk};
use librevault_util::path_normalize::{normalize, NormalizationError};
use librevault_util::secret::Secret;
use log::{debug, trace};
use rabin::{Rabin, RabinParams};
use rand::{thread_rng, Fill};
use sha3::digest::Update;
use sha3::{Digest, Sha3_224};
use std::fmt::{Display, Formatter};
use std::fs;
use std::io;
use std::io::{BufReader, ErrorKind, Read};
use std::path::{Path, PathBuf};

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/librevault.meta.v2.rs"));
}

type ObjectMeta = proto::ObjectMeta;
type ObjectKind = proto::ObjectKind;
type Chunk = proto::Chunk;

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

fn make_chunk(data: &[u8], secret: &Secret) -> Chunk {
    debug!("New chunk size: {}", data.len());

    let mut iv = vec![0u8; 16];
    iv.try_fill(&mut thread_rng()).unwrap();

    let symmetric_key = secret.get_symmetric_key().unwrap();

    Chunk {
        ciphertext_hash: Sha3_224::digest(&*encrypt_chunk(data, symmetric_key, &*iv)).to_vec(),
        iv,
        size: data.len() as u32,
    }
}

fn make_chunks_for_buf<R: Read>(reader: R, secret: &Secret) -> Result<Vec<Chunk>, IndexingError> {
    Rabin::new(reader, RabinParams::default())
        .map(|chunk| Ok(make_chunk(chunk?.as_slice(), secret)))
        .collect()
}

fn make_chunks(path: &Path, secret: &Secret) -> Result<Vec<Chunk>, IndexingError> {
    trace!("Trying to make chunks for: {:?}", path);

    let f = fs::File::open(path)?;
    let reader = BufReader::new(f);
    let chunks = make_chunks_for_buf(reader, secret)?;

    trace!("Total chunks for path {:?}: {}", path, chunks.len());

    Ok(chunks)
}

fn make_datastream(chunks: &[Chunk]) -> proto::DataStream {
    proto::DataStream {
        chunk_ids: chunks
            .iter()
            .map(|chunk| chunk.ciphertext_hash.clone())
            .collect(),
    }
}

fn make_objectmeta(
    path: &Path,
    root: &Path,
    secret: &Secret,
) -> Result<(ObjectMeta, Vec<Chunk>), IndexingError> {
    debug!(
        "Requested ObjectMeta indexing: path={:?} root={:?}",
        path, root
    );

    let mut objectmeta = ObjectMeta::default();

    let objectmeta_kind = make_kind(path, true)?;

    objectmeta.kind = objectmeta_kind as i32;
    objectmeta.normalized_path = normalize(path, root, true)?;

    let mut chunks = vec![];

    if objectmeta_kind == ObjectKind::File {
        chunks = make_chunks(path, secret)?;
        objectmeta
            .data_streams
            .insert(String::from(""), make_datastream(chunks.as_slice()));
    }

    debug!(
        "Got ObjectMeta for path={:?} root={:?} objectmeta={:?} chunks={:?}",
        path, root, objectmeta, chunks
    );

    Ok((objectmeta, chunks))
}

pub(crate) fn make_changeset(paths: &[&Path], root: &Path, secret: &Secret) {
    let mut chunks = vec![];
    let mut objectmetas = vec![];

    for path in paths {
        let result = make_objectmeta(path, root, secret);
        if let Ok((om_objectmeta, om_chunks)) = result {
            chunks.extend_from_slice(&*om_chunks);
            objectmetas.push(om_objectmeta);
        }
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
    } else if file_type.is_symlink() {
        Ok(ObjectKind::Symlink)
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
