use std::{ptr, slice, str};

#[macro_export]
macro_rules! convert_str {
    ($string:expr, $length:expr) => {
        str::from_utf8(unsafe { slice::from_raw_parts($string, $length) }).unwrap()
    };
    ($($string:expr, $length:expr), *) => {
        ( $(convert_str!($string, $length),)* )
    }
}

#[repr(C)]
pub struct FfiConstBuffer {
    str_p: *const u8,
    str_len: usize,
}

impl FfiConstBuffer {
    pub fn to_str(&self) -> &str {
        convert_str!(self.str_p, self.str_len)
    }
    pub fn as_slice(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.str_p, self.str_len) }
    }

    pub fn from_slice(s: &[u8]) -> Self {
        FfiConstBuffer {
            str_len: s.len(),
            str_p: Box::into_raw(Box::<[u8]>::from(s)) as *const u8,
        }
    }
}

impl Default for FfiConstBuffer {
    fn default() -> Self {
        FfiConstBuffer {
            str_len: 0,
            str_p: ptr::null(),
        }
    }
}

impl From<Vec<u8>> for FfiConstBuffer {
    fn from(v: Vec<u8>) -> Self {
        FfiConstBuffer {
            str_len: v.len(),
            str_p: Box::into_raw(v.into_boxed_slice()) as *const u8,
        }
    }
}

#[no_mangle]
pub extern "C" fn drop_ffi(buf: FfiConstBuffer) {
    unsafe { Box::from_raw(buf.str_p as *mut u8) };
}
