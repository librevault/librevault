use crate::ffi::FfiConstBuffer;

#[no_mangle]
pub extern "C" fn b58_encode(in_buf: FfiConstBuffer) -> FfiConstBuffer {
    FfiConstBuffer::from_slice(bs58::encode(in_buf.as_slice()).into_vec().as_slice())
}

#[no_mangle]
pub extern "C" fn b58_decode(in_buf: FfiConstBuffer) -> FfiConstBuffer {
    FfiConstBuffer::from_slice(
        bs58::decode(in_buf.as_slice())
            .into_vec()
            .unwrap()
            .as_slice(),
    )
}

#[no_mangle]
pub extern "C" fn b32_encode(in_buf: FfiConstBuffer) -> FfiConstBuffer {
    FfiConstBuffer::from_slice(
        base32::encode(
            base32::Alphabet::RFC4648 { padding: true },
            in_buf.as_slice(),
        )
        .as_bytes(),
    )
}

#[no_mangle]
pub extern "C" fn b32_decode(in_buf: FfiConstBuffer) -> FfiConstBuffer {
    FfiConstBuffer::from_slice(
        base32::decode(base32::Alphabet::RFC4648 { padding: true }, in_buf.to_str())
            .unwrap()
            .as_slice(),
    )
}
