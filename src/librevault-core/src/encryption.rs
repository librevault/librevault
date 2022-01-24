use crate::proto::meta::{EncryptedData, EncryptionMetadata};
use aes_gcm_siv::aead::{Aead, NewAead};
use aes_gcm_siv::{Aes256GcmSiv, Key, Nonce};
use rand::{thread_rng, Fill};

pub fn generate_rand_buf(size: usize) -> Vec<u8> {
    let mut buf = vec![0u8; size];
    buf.try_fill(&mut thread_rng()).unwrap();
    buf
}

pub fn encrypt(data: &[u8], key: &[u8], metadata: EncryptionMetadata) -> EncryptedData {
    let key = Key::from_slice(key);
    let cipher = Aes256GcmSiv::new(key);

    let nonce = Nonce::from_slice(metadata.iv.as_slice());
    let ciphertext = cipher.encrypt(nonce, data).unwrap();

    EncryptedData {
        metadata: Some(metadata),
        ciphertext,
    }
}

pub fn decrypt(data: &EncryptedData, key: &[u8]) -> Option<Vec<u8>> {
    let metadata = data.metadata.as_ref()?;

    let key = Key::from_slice(key);
    let cipher = Aes256GcmSiv::new(key);
    let nonce = Nonce::from_slice(metadata.iv.as_slice());

    cipher.decrypt(nonce, &*data.ciphertext).ok()
}
