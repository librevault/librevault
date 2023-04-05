use actix::prelude::*;
use directories::ProjectDirs;
use libp2p::identity::Keypair;
use libp2p::multiaddr::multiaddr;
use libp2p::{Multiaddr, Swarm};
use librevault_metadata::object::{ObjectMetadata, ObjectMetadataParams};
use std::net::{Ipv4Addr, Ipv6Addr};
use std::path::PathBuf;
use std::sync::Arc;
use tracing::{debug, info};

use crate::p2p::NodeBehaviour;
use settings::ConfigManager;

mod composed_event;
mod p2p;
mod settings;

struct BucketId(Vec<u8>);
struct ChunkId(Vec<u8>);

struct BucketOptions {
    root: PathBuf,
}

struct Bucket {
    opts: BucketOptions,
}

impl Bucket {
    fn new(opts: BucketOptions) -> Self {
        Self { opts }
    }
}

impl Actor for Bucket {
    type Context = Context<Self>;

    fn started(&mut self, ctx: &mut Self::Context) {
        info!("Bucket starting");
        for x in walkdir::WalkDir::new(&self.opts.root) {
            let metadata =
                ObjectMetadata::from_path(x.unwrap().path(), &ObjectMetadataParams::default())
                    .unwrap();
            println!("{:?}", metadata);
        }
    }
}

struct ObjectPointer {
    name: String,
    metadata: Vec<u8>,
    datastream: Vec<u8>,
}

struct PointerStorage {}
impl PointerStorage {}

struct MetadataStorage {}

fn initialize(id_keys: Keypair) -> Swarm<NodeBehaviour> {
    let mut swarm = p2p::make_swarm(id_keys.clone());

    // Add listeners
    let bound_multiaddrs: Vec<Multiaddr> = vec![
        multiaddr!(Ip4(Ipv4Addr::UNSPECIFIED), Tcp(0u16)),
        multiaddr!(Ip6(Ipv6Addr::UNSPECIFIED), Tcp(0u16)),
    ];
    for bound_multiaddr in bound_multiaddrs {
        swarm.listen_on(bound_multiaddr).unwrap();
    }
    swarm
}

#[actix::main]
async fn main() {
    const BANNER: &str = r#"
   __    __ _                                _ __
  / /   /_/ /_  ____ _____ _  __ ___  __  __/ / /_
 / /   __/ /_ \/ ___/ ___/ / / / __ \/ / / / / __/
/ /___/ / /_/ / /  / ___/\ \/ / /_/ / /_/ / / /___
\____/_/\____/_/  /____/  \__/_/ /_/\____/_/\____/
"#;
    print!("{BANNER}");

    tracing_subscriber::fmt::init();
    // console_subscriber::init();

    info!("Librevault v{}", env!("CARGO_PKG_VERSION"));

    let project_dirs = ProjectDirs::from("com", "librevault", "librevault").unwrap();
    let config_dir = project_dirs.config_dir();
    debug!("Config directory: {:?}", &config_dir);
    std::fs::create_dir_all(config_dir).expect("Could not create config directory");

    let bucket = Bucket::create(|_| Bucket::new(BucketOptions { root: ".".into() }));

    let settings = Arc::new(ConfigManager::new(config_dir).unwrap());

    tokio::select! {
        _ = tokio::signal::ctrl_c() => {},
    }
}
