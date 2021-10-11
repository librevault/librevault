use crate::ffi::FfiConstBuffer;

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
