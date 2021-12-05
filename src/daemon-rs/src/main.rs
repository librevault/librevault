use std::ffi::OsStr;
use std::path::Path;
use std::sync::Arc;

use directories::ProjectDirs;

use log::{debug, info};

use crate::p2p::run_server;
use crate::settings::ConfigManager;
use bucket::manager::BucketManager;

mod bucket;
mod grpc;
#[cfg(unix)]
mod lvfs;
mod p2p;
mod settings;

#[tokio::main]
async fn main() {
    const BANNER: &str = r#"
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

    tokio::spawn(grpc::run_grpc(buckets.clone(), settings.clone()));
    tokio::spawn(run_server(
        buckets.clone(),
        settings.clone(),
        buckets.get_event_channel(),
    ));

    // Add buckets only after passing to components
    for bucket_config in &settings.config().buckets {
        buckets.add_bucket(bucket_config.clone()).await;
    }

    #[cfg(unix)]
    {
        let filesystem = lvfs::LibrevaultFs::new(buckets.get_bucket_one().unwrap());

        let _ = fuse_mt::spawn_mount(
            fuse_mt::FuseMT::new(filesystem, 1),
            Path::new("/home/gamepad/lvfs"),
            vec![OsStr::new("-o"), OsStr::new("auto_unmount")].as_slice(),
        );
    }

    let _ = tokio::signal::ctrl_c().await;

    // let gateway = search_gateway(SearchOptions::default()).await.unwrap();
    // info!("Detected IGD gateway: {:?}", gateway);
}
