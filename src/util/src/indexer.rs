use std::str::FromStr;
use rand::{Fill, thread_rng};
use sha3::{Digest, Sha3_224};
use sha3::digest::Update;
use crate::secret::{OpaqueSecret, SecretError};
use log::{warn, debug};
use prost::Message;
use crate::aescbc::encrypt_chunk;

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/librevault.serialization.rs"));
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

fn c_populate_chunk(data: &[u8], secret: &str) -> Result<Vec<u8>, SecretError> {
    Ok(populate_chunk(data, &OpaqueSecret::from_str(secret)?).encode_to_vec())
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn c_populate_chunk(data: &[u8], secret: &str) -> Result<Vec<u8>>;
    }
}
