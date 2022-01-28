use crate::proto::meta::SignedRevision;
use kv::{Config, Raw, Store};
use multihash::{Code, MultihashDigest};
use prost::Message;
use std::path::{Path, PathBuf};
use tracing::log::info;

pub struct Index {
    store: Store,
}

impl Index {
    pub fn init(root: &Path) -> Self {
        let cfg = Config::new(root);
        let store = Store::new(cfg).unwrap();
        Self { store }
    }

    pub fn get_revision_by_hash(&self, hash: &[u8]) -> Option<Vec<u8>> {
        let stb_revision = self.store.bucket::<Raw, Raw>(Some("revisions")).unwrap();
        Some(stb_revision.get(hash).ok()??.to_vec())
    }

    fn get_head_revision_raw(&self) -> Option<Vec<u8>> {
        let stb_refs = self.store.bucket::<String, Raw>(Some("refs")).unwrap();
        let head_id = stb_refs.get("HEAD").ok()??.to_vec();

        self.get_revision_by_hash(&*head_id)
    }

    fn get_head_revision(&self) -> Option<SignedRevision> {
        SignedRevision::decode(&*self.get_head_revision_raw()?).ok()
    }

    pub fn put_head_revision(&self, revision: &SignedRevision) {
        let stb_refs = self.store.bucket::<String, Raw>(Some("refs")).unwrap();
        let stb_revision = self.store.bucket::<Raw, Raw>(Some("revisions")).unwrap();

        let revision_serialized = revision.encode_to_vec();
        let revision_hash = Code::Sha3_256.digest(&*revision_serialized).to_bytes();

        stb_revision.set(revision_hash.clone(), revision_serialized);
        info!(
            "Added revision {} to index",
            hex::encode(revision_hash.clone())
        );
        stb_refs.set("HEAD", revision_hash.clone());
        info!("Set ref HEAD to {}", hex::encode(revision_hash));
    }

    pub fn print_info(&self) {
        let stb_refs = self.store.bucket::<String, Raw>(Some("refs")).unwrap();

        for _ref in stb_refs.iter() {
            let item = _ref.ok().unwrap();
            let refname: String = item.key().unwrap();
            let refhash: Raw = item.value().unwrap();
            let refhash = refhash.to_vec();
            println!("ref: {refname}, hash: {}", hex::encode(refhash))
        }
    }

    pub fn default_index_file(root: &Path) -> PathBuf {
        root.join(".librevault/index.db")
    }
}
