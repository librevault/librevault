use aes::Aes256;
use block_modes::block_padding::Pkcs7;
use block_modes::{BlockMode, Cbc};

use crate::ffi::FfiConstBuffer;

type Aes256Cbc = Cbc<Aes256, Pkcs7>;

#[no_mangle]
pub extern "C" fn encrypt_aes256(
    message: FfiConstBuffer,
    key: FfiConstBuffer,
    iv: FfiConstBuffer,
) -> FfiConstBuffer {
    let (message, key, iv) = (message.as_slice(), key.as_slice(), iv.as_slice());
    let cipher = Aes256Cbc::new_from_slices(key, iv).unwrap();
    FfiConstBuffer::from(cipher.encrypt_vec(message))
}

#[no_mangle]
pub extern "C" fn decrypt_aes256(
    message: FfiConstBuffer,
    key: FfiConstBuffer,
    iv: FfiConstBuffer,
) -> FfiConstBuffer {
    let (message, key, iv) = (message.as_slice(), key.as_slice(), iv.as_slice());
    let cipher = Aes256Cbc::new_from_slices(key, iv).unwrap();
    FfiConstBuffer::from(cipher.decrypt_vec(message).unwrap())
}

pub fn encrypt_chunk(message: &[u8], key: &[u8], iv: &[u8]) -> Vec<u8> {
    Aes256Cbc::new_from_slices(key, iv)
        .unwrap()
        .encrypt_vec(message)
}
