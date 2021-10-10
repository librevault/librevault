mod ffi;
mod logger;
mod nodekey;
mod rabin;
mod b58;
mod aescbc;

use std::os::raw::c_char;
use crate::ffi::FfiConstBuffer;
use luhn::Luhn;
use rand::prelude::*;
use rand::Fill;

#[no_mangle]
pub extern "C" fn fill_random(array: *mut u8, len: usize) {
    let slice = unsafe { std::slice::from_raw_parts_mut(array, len) };

    let mut rng = thread_rng();
    slice.try_fill(&mut rng).unwrap();
}

#[no_mangle]
pub extern "C" fn calc_luhnmod58(buf: FfiConstBuffer) -> c_char {
    Luhn::new("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz")
        .unwrap()
        .generate(buf.to_str())
        .unwrap() as c_char
}
