pub mod encryption;
pub mod index;
pub mod indexer;
pub mod meta_ext;
pub mod secret;

pub mod proto {
    pub use self::meta::*;
    pub mod meta {
        include!(concat!(env!("OUT_DIR"), "/librevault.meta.v2.rs"));
    }

    pub use self::protocol::*;
    pub mod protocol {
        include!(concat!(env!("OUT_DIR"), "/librevault.protocol.v1.rs"));
    }

    pub use self::controller::*;
    pub mod controller {
        pub const FILE_DESCRIPTOR_SET: &[u8] = include_bytes!(concat!(
            env!("OUT_DIR"),
            concat!("/librevault.controller.v1.bin")
        ));
        include!(concat!(env!("OUT_DIR"), "/librevault.controller.v1.rs"));
    }
}
