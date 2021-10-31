use std::path::{Path, PathBuf};
use std::sync::Arc;

use directories::ProjectDirs;
use igd::aio::search_gateway;
use igd::SearchOptions;
use log::{debug, info};

use crate::settings::ConfigManager;
use bucket::BucketManager;
use librevault_util::nodekey::nodekey_write_new;
use librevault_util::secret::Secret;

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

    env_logger::init();

    const VERSION: &str = env!("CARGO_PKG_VERSION");
    info!("Librevault v{}", VERSION);

    let project_dirs = ProjectDirs::from("com", "librevault", "librevault").unwrap();
    let config_dir = project_dirs.config_dir();
    debug!("Config directory: {:?}", &config_dir);
    std::fs::create_dir_all(&config_dir).expect("Could not create config directory");

    let settings = Arc::new(ConfigManager::new(config_dir).unwrap());

    let buckets = Arc::new(BucketManager::new());
    for bucket_config in &settings.config().buckets {
        buckets.add_bucket(bucket_config.clone()).await;
    }

    nodekey_write_new(config_dir.join("key.pem").to_str().unwrap());
    tokio::spawn(grpc::run_grpc(buckets.clone(), settings.clone()));

    let _ = tokio::signal::ctrl_c().await;

    // discover_mcast().await;

    // let gateway = search_gateway(SearchOptions::default()).await.unwrap();
    // info!("Detected IGD gateway: {:?}", gateway);
}
