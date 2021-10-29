use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::{Arc, RwLock};

use directories::ProjectDirs;
use igd::aio::search_gateway;
use igd::SearchOptions;
use log::{debug, info};
use tonic::transport::Server;

use bucket::{BucketConfig, BucketManager};
use librevault_util;
use librevault_util::nodekey::nodekey_write_new;
use librevault_util::secret::Secret;

use crate::discover::discover_mcast;

mod bucket;
mod discover;
mod grpc;
mod settings;

#[tokio::main]
async fn main() {
    const BANNER: &'static str = r#"
   __    __ _                                _ __
  / /   /_/ /_  ____ _____ _  __ ___  __  __/ / /_
 / /   __/ /_ \/ ___/ ___/ / / / __ \/ / / / / __/
/ /___/ / /_/ / /  / ___/\ \/ / /_/ / /_/ / / /___
\____/_/\____/_/  /____/  \__/_/ /_/\____/_/\____/
"#;
    print!("{}", BANNER);

    simple_logger::SimpleLogger::new().env().init().unwrap();

    const VERSION: &str = env!("CARGO_PKG_VERSION");
    info!("Librevault v{}", VERSION);

    let project_dirs = ProjectDirs::from("com", "librevault", "librevault").unwrap();
    let config_dir = project_dirs.config_dir();
    debug!("Config directory: {:?}", &config_dir);
    std::fs::create_dir_all(&config_dir).expect("Could not create config directory");

    let settings = settings::init_settings(config_dir).unwrap();

    librevault_util::nodekey::nodekey_write_new(config_dir.join("key.pem").to_str().unwrap());

    let buckets = BucketManager::new();

    let bucket = BucketConfig {
        secret: Secret::new(),
        path: PathBuf::from(Path::new("/home/gamepad/LibrevaultRs")),
    };
    buckets.add_bucket(bucket).await;

    tokio::spawn(grpc::launch_grpc());

    tokio::signal::ctrl_c().await;

    // discover_mcast().await;

    // let gateway = search_gateway(SearchOptions::default()).await.unwrap();
    // info!("Detected IGD gateway: {:?}", gateway);
}
