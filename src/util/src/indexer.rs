use crate::aescbc::encrypt_aes256;
use crate::aescbc::encrypt_chunk;
use crate::index::SignedMeta;
use crate::path_normalize::normalize;
use crate::rabin::Rabin;
use crate::secret::Secret;
use log::{debug, trace};
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;
use prost::Message;
use rand::{thread_rng, Fill};
use sha3::digest::Update;
use sha3::{Digest, Sha3_224};
use std::fmt::{Display, Formatter};
use std::fs;
use std::io;
use std::io::{BufReader, ErrorKind, Read};
use std::path::Path;
use std::str::FromStr;
use std::time::SystemTime;

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/librevault.serialization.rs"));
}

#[derive(FromPrimitive)]
enum ObjectType {
    Tombstone = 0,
    File = 1,
    Directory = 2,
    Symlink = 3,
}

#[derive(Debug)]
pub enum IndexingError {
    IoError(io::Error),
    UnsupportedType,
}

impl From<io::Error> for IndexingError {
    fn from(error: io::Error) -> Self {
        IndexingError::IoError(error)
    }
}

impl Display for IndexingError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

fn kmac_sha3_224(key: &[u8], data: &[u8]) -> Vec<u8> {
    let mut hasher = Sha3_224::new();
    Update::update(&mut hasher, key);
    Update::update(&mut hasher, data);
    hasher.finalize().to_vec()
}

fn populate_chunk(data: &[u8], secret: &Secret) -> proto::meta::file_metadata::Chunk {
    debug!("New chunk size: {}", data.len());

    let mut iv = vec![0u8; 16];
    iv.try_fill(&mut thread_rng()).unwrap();

    let symmetric_key = secret.get_symmetric_key().unwrap();

    proto::meta::file_metadata::Chunk {
        ct_hash: Sha3_224::digest(&*encrypt_chunk(data, symmetric_key, &*iv)).to_vec(),
        iv,
        size: data.len() as u32,
        pt_hmac: kmac_sha3_224(symmetric_key, data),
    }
}

fn make_chunks(
    path: &Path,
    secret: &Secret,
) -> Result<Vec<proto::meta::file_metadata::Chunk>, IndexingError> {
    trace!("Trying to make chunks for: {:?}", path);

    let f = fs::File::open(path)?;
    let reader = BufReader::new(f);

    let mut chunks = vec![];

    let mut chunker = Rabin::default();

    let mut chunk_data = Vec::with_capacity(chunker.maxsize as usize);

    for b in reader.bytes() {
        let b = b?;
        chunk_data.push(b);
        if chunker.rabin_next_chunk(b) {
            let chunk = populate_chunk(chunk_data.as_slice(), secret);
            // Found a chunk
            chunk_data.truncate(0);

            chunks.push(chunk);
        }
    }

    if !chunk_data.is_empty() {
        let chunk = populate_chunk(chunk_data.as_slice(), secret);
        chunks.push(chunk);
    }

    trace!("Total chunks for path {:?}: {}", path, chunks.len());

    Ok(chunks)
}

fn make_encrypted(data: &[u8], secret: &Secret) -> proto::AesCbc {
    let mut iv = vec![0u8; 16];
    iv.try_fill(&mut thread_rng()).unwrap();

    proto::AesCbc {
        ct: encrypt_aes256(data, secret.get_symmetric_key().unwrap(), &*iv),
        iv,
    }
}

fn make_type(path: &Path, preserve_symlinks: bool) -> Result<ObjectType, IndexingError> {
    let metadata = match preserve_symlinks {
        true => fs::symlink_metadata(path),
        false => fs::metadata(path),
    };

    if let Err(e) = metadata {
        return match e.kind() {
            ErrorKind::NotFound => Ok(ObjectType::Tombstone),
            _ => Err(IndexingError::from(e)),
        };
    }

    let file_type = metadata.unwrap().file_type();
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

pub fn make_meta(path: &Path, root: &Path, secret: &Secret) -> Result<proto::Meta, IndexingError> {
    let mut meta = proto::Meta::default();

    let path_norm = normalize(path, root, true).unwrap();
    meta.path_id = kmac_sha3_224(secret.get_symmetric_key().unwrap(), path_norm.as_slice());
    meta.path = Some(make_encrypted(path_norm.as_slice(), secret));
    meta.meta_type = make_type(path, true).unwrap() as u32;
    meta.revision = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap()
        .as_secs() as i64;

    if meta.meta_type != (ObjectType::Tombstone as u32) {
        let metadata = fs::metadata(path).unwrap();
        let generic_metadata = proto::meta::GenericMetadata {
            mtime: metadata
                .modified()
                .unwrap()
                .duration_since(SystemTime::UNIX_EPOCH)
                .unwrap()
                .as_nanos() as i64,
            windows_attrib: 0, // TODO: Windows attributes
            uid: 0,            // TODO: Unix attributes
            gid: 0,            // TODO: Unix attributes
            mode: 0,           // TODO: Unix attributes
        };
        meta.generic_metadata = Some(generic_metadata);
    }

    meta.type_specific_metadata = match FromPrimitive::from_u32(meta.meta_type) {
        Some(ObjectType::File) => Some(proto::meta::TypeSpecificMetadata::FileMetadata(
            proto::meta::FileMetadata {
                algorithm_type: 0,
                strong_hash_type: 0,
                max_chunksize: 1024 * 1024,
                min_chunksize: 8 * 1024 * 1024,
                chunks: make_chunks(path, secret)?,
            },
        )),
        // Some(ObjectType::DIRECTORY) => {}
        // Some(ObjectType::SYMLINK) => {}
        _ => None,
    };

    Ok(meta)
}

pub fn make_tombstone(path_norm: &[u8], secret: &Secret) -> Result<proto::Meta, IndexingError> {
    let current_timestamp = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap();

    let mut meta = proto::Meta::default();

    meta.path_id = kmac_sha3_224(secret.get_symmetric_key().unwrap(), path_norm);
    meta.path = Some(make_encrypted(path_norm, secret));
    meta.meta_type = ObjectType::Tombstone as u32;
    meta.revision = current_timestamp.as_secs() as i64;

    Ok(meta)
}

pub fn make_meta_empty_file(
    path_norm: &[u8],
    secret: &Secret,
) -> Result<proto::Meta, IndexingError> {
    let current_timestamp = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap();

    let mut meta = proto::Meta::default();

    meta.path_id = kmac_sha3_224(secret.get_symmetric_key().unwrap(), path_norm);
    meta.path = Some(make_encrypted(path_norm, secret));
    meta.meta_type = ObjectType::File as u32;
    meta.revision = current_timestamp.as_secs() as i64;

    meta.generic_metadata = Some(proto::meta::GenericMetadata {
        mtime: current_timestamp.as_nanos() as i64,
        windows_attrib: 0, // TODO: Windows attributes
        uid: 0,            // TODO: Unix attributes
        gid: 0,            // TODO: Unix attributes
        mode: 0,           // TODO: Unix attributes
    });

    meta.type_specific_metadata = Some(proto::meta::TypeSpecificMetadata::FileMetadata(
        proto::meta::FileMetadata {
            algorithm_type: 0,
            strong_hash_type: 0,
            max_chunksize: 1024 * 1024,
            min_chunksize: 8 * 1024 * 1024,
            chunks: vec![],
        },
    ));

    Ok(meta)
}

pub fn sign_meta(meta: &proto::Meta, secret: &Secret) -> SignedMeta {
    let serialized_meta = meta.encode_to_vec();
    let signature = secret.sign(&*serialized_meta).unwrap();
    SignedMeta {
        meta: serialized_meta,
        signature,
    }
}

fn c_make_chunks(path: &str, secret: &str) -> Result<Vec<u8>, IndexingError> {
    let chunks = make_chunks(Path::new(path), &Secret::from_str(secret).unwrap())?;
    let chunks: Vec<String> = chunks
        .into_iter()
        .map(|chunk| base64::encode(chunk.encode_to_vec()))
        .collect();

    let j = serde_json::to_vec(&serde_json::Value::from(chunks)).unwrap();
    trace!("Chunks_json: {}", String::from_utf8(j.clone()).unwrap());

    Ok(j)
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn c_make_chunks(path: &str, secret: &str) -> Result<Vec<u8>>;
    }
}
