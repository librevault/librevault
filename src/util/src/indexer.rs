use crate::aescbc::encrypt_chunk;
use crate::indexer::IndexingError::IoError;
use crate::rabin::{rabin_init, rabin_next_chunk, Rabin};
use crate::secret::{OpaqueSecret, SecretError};
use log::{debug, trace, warn};
use prost::Message;
use rand::{thread_rng, Fill};
use sha3::digest::Update;
use sha3::{Digest, Sha3_224};
use std::fmt::{Display, Formatter};
use std::fs;
use std::io;
use std::io::{BufReader, Read};
use std::path::Path;
use std::str::FromStr;

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/librevault.serialization.rs"));
}

#[derive(Debug)]
pub enum IndexingError {
    IoError { io_error: io::Error },
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

fn populate_chunk(data: &[u8], secret: &OpaqueSecret) -> proto::meta::file_metadata::Chunk {
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
    secret: &OpaqueSecret,
) -> Result<Vec<proto::meta::file_metadata::Chunk>, IndexingError> {
    trace!("Trying to make chunks for: {:?}", path);

    let f = match fs::File::open(path) {
        Ok(f) => f,
        Err(e) => return Err(IoError { io_error: e }),
    };
    let reader = BufReader::new(f);

    let mut chunks = vec![];

    let mut chunker = Rabin::init();
    rabin_init(&mut chunker);

    let mut chunk_data = Vec::with_capacity(chunker.maxsize as usize);

    for b in reader.bytes() {
        if let Ok(b) = b {
            chunk_data.push(b);
            if rabin_next_chunk(&mut chunker, b) {
                let chunk = populate_chunk(chunk_data.as_slice(), secret);
                // Found a chunk
                chunk_data.truncate(0);

                chunks.push(chunk);
            }
        } else if let Err(e) = b {
            warn!("I/O Error: {:?}", e);
            return Err(IndexingError::IoError { io_error: e });
        }
    }

    if !chunk_data.is_empty() {
        let chunk = populate_chunk(chunk_data.as_slice(), secret);
        chunks.push(chunk);
    }

    trace!("Total chunks for path {:?}: {}", path, chunks.len());

    Ok(chunks)
}

fn c_make_chunks(path: &str, secret: &str) -> Result<Vec<u8>, IndexingError> {
    let chunks = make_chunks(Path::new(path), &OpaqueSecret::from_str(secret).unwrap())?;
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
