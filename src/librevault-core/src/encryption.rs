use crate::proto::encryption_metadata::{AesGcmSivAlgorithm, Algorithm};
use crate::proto::meta::{EncryptedData, EncryptionMetadata};
use aes_gcm_siv::aead::Aead;
use aes_gcm_siv::{Aes256GcmSiv, Key, KeyInit, Nonce};
use rand::Rng;

pub trait Initialize: Sized {
    fn initialize() -> Self;
}

impl Initialize for AesGcmSivAlgorithm {
    fn initialize() -> Self {
        let mut nonce = vec![0u8; 12];
        rand::thread_rng().fill(&mut nonce[..]);
        Self { nonce }
    }
}

pub trait EncryptedDataExt: Sized {
    fn encrypt(data: &[u8], key: &[u8], metadata: EncryptionMetadata) -> Self;
    fn decrypt(&self, key: &[u8]) -> Option<Vec<u8>>;
}

impl EncryptedDataExt for EncryptedData {
    fn encrypt(data: &[u8], key: &[u8], metadata: EncryptionMetadata) -> Self {
        match &metadata.algorithm {
            Some(Algorithm::AesGcmSiv(AesGcmSivAlgorithm { nonce })) => {
                let key = Key::<Aes256GcmSiv>::from_slice(key);
                let cipher = Aes256GcmSiv::new(key);

                let nonce = Nonce::from_slice(nonce.as_slice());
                let ciphertext = cipher.encrypt(nonce, data).unwrap();

                Self {
                    metadata: Some(metadata),
                    ciphertext,
                }
            }
            _ => {
                panic!("Unsupported algorithm: {:?}", metadata.algorithm)
            }
        }
    }

    fn decrypt(&self, key: &[u8]) -> Option<Vec<u8>> {
        let Some(metadata) = &self.metadata else {
            panic!("Metadata is not set");
        };

        match &metadata.algorithm {
            Some(Algorithm::AesGcmSiv(AesGcmSivAlgorithm { nonce })) => {
                let key = Key::<Aes256GcmSiv>::from_slice(key);
                let cipher = Aes256GcmSiv::new(key);
                let nonce = Nonce::from_slice(nonce.as_slice());

                cipher.decrypt(nonce, &*self.ciphertext).ok()
            }
            _ => {
                panic!("Unsupported algorithm: {:?}", metadata.algorithm)
            }
        }
    }
}
