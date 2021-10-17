use figment::providers::{Format, Json, Serialized};
use figment::Figment;
use log::debug;
use serde::{Deserialize, Serialize};
use std::error::Error;
use std::fs;
use std::path::Path;

#[derive(Serialize, Deserialize)]
pub struct RootConfig {
    client_name: String,
    control_listen: u16,
    p2p_listen: u16,
    p2p_download_slots: u16,
    p2p_request_timeout: u16,
    p2p_block_size: u32,
    natpmp_enabled: bool,
    natpmp_lifetime: u32,
    upnp_enabled: bool,
    predef_repeat_interval: u32,
    multicast_enabled: bool,
    multicast_repeat_interval: u32,
    mainline_dht_enabled: bool,
    mainline_dht_port: u16,
    mainline_dht_routers: Vec<String>,
}

impl Default for RootConfig {
    fn default() -> RootConfig {
        RootConfig {
            client_name: "Librevault".to_string(),
            control_listen: 42346,
            p2p_listen: 42345,
            p2p_download_slots: 10,
            p2p_request_timeout: 10,
            p2p_block_size: 32768,
            natpmp_enabled: true,
            natpmp_lifetime: 3600,
            upnp_enabled: true,
            predef_repeat_interval: 30,
            multicast_enabled: true,
            multicast_repeat_interval: 30,
            mainline_dht_enabled: true,
            mainline_dht_port: 42347,
            mainline_dht_routers: vec![
                "router.utorrent.com:6881".to_string(),
                "router.bittorrent.com:6881".to_string(),
                "dht.transmissionbt.com:6881".to_string(),
                "dht.aelitis.com:6881".to_string(),
                "dht.libtorrent.org:25401".to_string(),
            ],
        }
    }
}

pub(crate) fn init_settings(config_base: &Path) -> Result<RootConfig, Box<dyn Error>> {
    fs::create_dir_all(config_base)?;

    let config_file = config_base.join("globals.json");
    debug!(
        "Reading config file from: {}",
        &config_file.to_str().unwrap()
    );

    Ok(Figment::from(Serialized::defaults(RootConfig::default()))
        .merge(Json::file(config_file))
        .extract()?)
}
