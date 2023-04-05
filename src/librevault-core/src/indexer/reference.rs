use std::hash::{Hash, Hasher};

use multihash::MultihashDigest;
use prost::Message;

use crate::proto::ReferenceHash;

pub trait ReferenceExt {
    fn from_serialized(serialized: &[u8]) -> Self;
    fn hex(&self) -> String;
}

impl ReferenceExt for ReferenceHash {
    fn from_serialized(serialized: &[u8]) -> Self {
        Self {
            multihash: multihash::Code::Sha3_256.digest(serialized).to_bytes(),
        }
    }

    fn hex(&self) -> String {
        hex::encode(&self.multihash)
    }
}

pub trait ReferenceMaker: Message + Sized {
    fn reference(&self) -> (Vec<u8>, ReferenceHash) {
        let serialized = self.encode_to_vec();
        let reference = ReferenceHash::from_serialized(&serialized);
        (serialized, reference)
    }
}

impl AsRef<[u8]> for ReferenceHash {
    fn as_ref(&self) -> &[u8] {
        &self.multihash
    }
}

impl Eq for ReferenceHash {}
impl Hash for ReferenceHash {
    fn hash<H: Hasher>(&self, state: &mut H) {
        state.write(self.multihash.as_slice())
    }
}
