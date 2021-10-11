mod aescbc;
mod b58;
mod ffi;
mod logger;
mod nodekey;
mod rabin;
mod secret;

use crate::ffi::FfiConstBuffer;
use luhn::Luhn;
use rand::prelude::*;
use rand::Fill;
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn fill_random(array: *mut u8, len: usize) {
    let slice = unsafe { std::slice::from_raw_parts_mut(array, len) };

    let mut rng = thread_rng();
    slice.try_fill(&mut rng).unwrap();
}
