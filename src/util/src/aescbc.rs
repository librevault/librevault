use aes::Aes256;
use block_modes::block_padding::Pkcs7;
use block_modes::{BlockMode, Cbc};

use crate::ffi::FfiConstBuffer;

type Aes256Cbc = Cbc<Aes256, Pkcs7>;

fn encrypt_aes256(message: &[u8], key: &[u8], iv: &[u8]) -> Vec<u8> {
    Aes256Cbc::new_from_slices(key, iv)
        .unwrap()
        .encrypt_vec(message)
}

fn decrypt_aes256(message: &[u8], key: &[u8], iv: &[u8]) -> Vec<u8> {
    Aes256Cbc::new_from_slices(key, iv)
        .unwrap()
        .decrypt_vec(message)
        .unwrap()
}

pub fn encrypt_chunk(message: &[u8], key: &[u8], iv: &[u8]) -> Vec<u8> {
    Aes256Cbc::new_from_slices(key, iv)
        .unwrap()
        .encrypt_vec(message)
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn encrypt_aes256(message: &[u8], key: &[u8], iv: &[u8]) -> Vec<u8>;
        fn decrypt_aes256(message: &[u8], key: &[u8], iv: &[u8]) -> Vec<u8>;
    }
}
