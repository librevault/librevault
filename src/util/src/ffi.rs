#[macro_export]
macro_rules! convert_str {
    ($string:expr, $length:expr) => {
        std::str::from_utf8(unsafe { std::slice::from_raw_parts($string, $length) }).unwrap()
    };
    ($($string:expr, $length:expr), *) => {
        ( $(convert_str!($string, $length),)* )
    }
}

#[repr(C)]
pub struct FfiString {
    str_p: *const u8,
    str_len: usize,
}

impl FfiString {
    pub fn to_str(&self) -> &str {
        convert_str!(self.str_p, self.str_len)
    }
}
