mod settings;

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::{Arc, RwLock};
use log::info;
use librevault_util;


struct Bucket {
    secret: librevault_util::secret::OpaqueSecret,
    root: PathBuf,
    index: librevault_util::index::Index,
}


struct BucketCollection {
    buckets_byid: HashMap<Vec<u8>, Arc<Bucket>>,
}

impl BucketCollection {
    fn new() -> BucketCollection {
        BucketCollection {
            buckets_byid: HashMap::new(),
        }
    }
}

struct BucketManager {
    buckets: Arc<RwLock<BucketCollection>>,
}

impl BucketManager {
    pub fn new() -> Self {
        BucketManager {
            buckets: Arc::new(RwLock::new(BucketCollection::new())),
        }
    }
}


fn main() {
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

    let config_path = Path::new("~/.librevault/Librevault").canonicalize().unwrap();
    let settings = settings::init_settings(config_path.as_path()).unwrap();

    // librevault_util::nodekey_write_new();
}
