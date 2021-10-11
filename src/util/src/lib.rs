mod aescbc;
mod b58;
mod ffi;
mod logger;
mod nodekey;
mod rabin;
mod secret;
mod path_normalize;

use rand::prelude::*;
use rand::Fill;

#[no_mangle]
pub extern "C" fn fill_random(array: *mut u8, len: usize) {
    let slice = unsafe { std::slice::from_raw_parts_mut(array, len) };

    let mut rng = thread_rng();
    slice.try_fill(&mut rng).unwrap();
}
