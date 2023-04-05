pub mod encryption;
pub mod indexer;
pub mod path;
pub mod secret;

pub mod proto {
    pub use self::meta::*;
    pub mod meta {
        pub use self::v2::*;
        pub mod v2 {
            include!(concat!(env!("OUT_DIR"), "/librevault.meta.v2.rs"));
        }
    }

    pub use self::protocol::*;
    pub mod protocol {
        pub use self::v1::*;
        pub mod v1 {
            include!(concat!(env!("OUT_DIR"), "/librevault.protocol.v1.rs"));
        }
    }

    pub use self::internal::*;
    pub mod internal {
        pub use self::v2::*;
        pub mod v2 {
            include!(concat!(env!("OUT_DIR"), "/librevault.internal.v1.rs"));
        }
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
