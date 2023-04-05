use crate::encryption::{EncryptedDataExt, Initialize};
use crate::indexer::reference::{ReferenceExt, ReferenceMaker};
use crate::proto::encryption_metadata::{AesGcmSivAlgorithm, Algorithm};
use crate::proto::{ChunkMetadata, EncryptedData, EncryptionMetadata, ReferenceHash};
use tracing::debug;

impl ReferenceMaker for ChunkMetadata {}

pub trait ChunkMetadataIndexer: Sized {
    fn compute_plaintext_hash(data: &[u8]) -> ReferenceHash;
    fn compute_from_buf(data: &[u8], key: &[u8], kid: u32) -> Self;
}

impl ChunkMetadataIndexer for ChunkMetadata {
    fn compute_plaintext_hash(data: &[u8]) -> ReferenceHash {
        ReferenceHash::from_serialized(data)
    }

    fn compute_from_buf(data: &[u8], key: &[u8], kid: u32) -> Self {
        let algorithm = AesGcmSivAlgorithm::initialize();
        let encryption_metadata = EncryptionMetadata {
            algorithm: Some(Algorithm::AesGcmSiv(algorithm)),
            kid,
        };

        let encrypted_data = EncryptedData::encrypt(data, key, encryption_metadata);
        let ciphertext_hash = ReferenceHash::from_serialized(&encrypted_data.ciphertext);

        let plaintext_hash = Self::compute_plaintext_hash(data);

        debug!("New chunk size: {}, hash: {ciphertext_hash:?}", data.len());

        ChunkMetadata {
            chunk: Some(ciphertext_hash),
            chunk_pt: Some(plaintext_hash),
            encryption_metadata: encrypted_data.metadata,
            size: data.len() as u32,
        }
    }
}
