extern crate core;

use std::sync::Arc;

use directories::ProjectDirs;
use tracing::{debug, info};

use settings::{BucketConfig, ConfigManager};

use crate::engine::Engine;

mod engine;
mod p2p;
mod settings;
mod watcher;

#[tokio::main]
async fn main() {
    const BANNER: &str = r#"
   __    __ _                                _ __
  / /   /_/ /_  ____ _____ _  __ ___  __  __/ / /_
 / /   __/ /_ \/ ___/ ___/ / / / __ \/ / / / / __/
/ /___/ / /_/ / /  / ___/\ \/ / /_/ / /_/ / / /___
\____/_/\____/_/  /____/  \__/_/ /_/\____/_/\____/
"#;
    print!("{BANNER}");

    // env_logger::init();
    tracing_subscriber::fmt::init();
    // console_subscriber::init();

    info!("Librevault v{}", env!("CARGO_PKG_VERSION"));

    let project_dirs = ProjectDirs::from("com", "librevault", "librevault").unwrap();
    let config_dir = project_dirs.config_dir();
    debug!("Config directory: {:?}", &config_dir);
    std::fs::create_dir_all(&config_dir).expect("Could not create config directory");

    let settings = Arc::new(ConfigManager::new(config_dir).unwrap());

    let mut engine = Engine::new();
    for bucket_config in &settings.config().buckets {
        engine.add_bucket(bucket_config.clone()).await;
    }

    engine.start().await;

    let _ = tokio::signal::ctrl_c().await;

    // let gateway = search_gateway(SearchOptions::default()).await.unwrap();
    // info!("Detected IGD gateway: {:?}", gateway);
}
