use std::fs;
use std::path::{Path, PathBuf};

use libp2p_identity::Keypair;
use tracing::info;

pub struct IdentityKeeper {
    identity_path: PathBuf,
}

impl IdentityKeeper {
    pub fn new(identity_path: &Path) -> Self {
        Self {
            identity_path: identity_path.into(),
        }
    }

    pub fn generate(&self) -> Keypair {
        let keypair = Keypair::generate_ed25519();
        fs::write(&self.identity_path, keypair.to_protobuf_encoding().unwrap()).unwrap();
        info!("Generated new identity in \"{:?}\"", &self.identity_path);
        keypair
    }

    pub fn try_get(&self) -> Option<Keypair> {
        let proto = fs::read(&self.identity_path).ok()?;
        Keypair::from_protobuf_encoding(&proto).ok()
    }

    pub fn get(&self) -> Keypair {
        self.try_get().unwrap_or_else(|| self.generate())
    }
}
