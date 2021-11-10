use figment::providers::{Format, Serialized, Toml};
use figment::Figment;
use librevault_util::secret::Secret;
use log::debug;
use serde::{Deserialize, Serialize};
use std::error::Error;
use std::fs;
use std::net::SocketAddr;
use std::path::{Path, PathBuf};

#[derive(Serialize, Deserialize, Clone)]
pub struct BucketConfig {
    pub path: PathBuf,
    pub secret: Secret,
}

#[derive(Serialize, Deserialize, Clone)]
pub(crate) struct ControllerConfig {
    pub(crate) bind: SocketAddr,
}

#[derive(Serialize, Deserialize, Clone)]
pub(crate) struct P2PConfig {
    pub(crate) bind: Vec<SocketAddr>,
}

#[derive(Serialize, Deserialize)]
pub(crate) struct RootConfig {
    pub(crate) client_name: String,
    pub(crate) buckets: Vec<BucketConfig>,
    pub(crate) controller: ControllerConfig,
    pub(crate) p2p: P2PConfig,
}

impl Default for RootConfig {
    fn default() -> Self {
        RootConfig {
            client_name: "Librevault".to_string(),
            buckets: vec![],
            controller: {
                ControllerConfig {
                    bind: "[::1]:42346".parse().unwrap(),
                }
            },
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
