mod aescbc;
mod enc_storage;
mod index;
mod indexer;
mod logger;
mod nodekey;
mod path_normalize;
mod rabin;
mod secret;

use rand::prelude::*;
use rand::Fill;

fn fill_random(array: &mut [u8]) {
    let mut rng = thread_rng();
    array.try_fill(&mut rng).unwrap();
}

fn b32_encode(in_buf: &[u8]) -> String {
    base32::encode(base32::Alphabet::RFC4648 { padding: true }, in_buf)
}

fn b32_decode(in_buf: &str) -> Vec<u8> {
    base32::decode(base32::Alphabet::RFC4648 { padding: true }, in_buf).unwrap()
}

#[cxx::bridge]
mod ff {
    extern "Rust" {
        fn fill_random(array: &mut [u8]);
        fn b32_encode(in_buf: &[u8]) -> String;
        fn b32_decode(in_buf: &str) -> Vec<u8>;
    }
}
