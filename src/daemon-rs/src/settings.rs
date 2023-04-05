use figment::providers::{Format, Serialized, Toml};
use figment::Figment;
use serde::{Deserialize, Serialize};
use std::error::Error;
use std::fs;
use std::net::SocketAddr;
use std::path::{Path, PathBuf};
use tracing::debug;

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct BucketConfig {
    pub path: PathBuf,
}

#[derive(Serialize, Deserialize, Clone)]
pub struct P2PConfig {
    pub bind: Vec<SocketAddr>,
}

#[derive(Serialize, Deserialize)]
pub struct RootConfig {
    pub client_name: String,
    pub buckets: Vec<BucketConfig>,
    pub p2p: P2PConfig,
}

impl Default for RootConfig {
    fn default() -> Self {
        RootConfig {
            client_name: "Librevault".to_string(),
            buckets: vec![],
            p2p: {
                P2PConfig {
                    bind: vec!["0.0.0.0:0".parse().unwrap(), "[::]:0".parse().unwrap()],
                }
            },
        }
    }
}

pub struct ConfigManager {
    config: RootConfig,
}

impl ConfigManager {
    pub(crate) fn new(config_base: &Path) -> Result<ConfigManager, Box<dyn Error>> {
        fs::create_dir_all(config_base)?;

        let config_file = config_base.join("config.toml");
        debug!(
            "Reading config file from: {}",
            &config_file.to_str().unwrap()
        );

        let config = Figment::from(Serialized::defaults(RootConfig::default()))
            .merge(Toml::file(config_file))
            .extract()?;

        Ok(ConfigManager { config })
    }

    pub(crate) fn config(&self) -> &RootConfig {
        &self.config
    }
}
