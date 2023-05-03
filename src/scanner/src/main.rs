use std::fs;
use std::path::Path;
use std::sync::Arc;
use std::time::Duration;

use actix::{Actor, AsyncContext};
use bucket::actor::ReindexAll;
use clap::{Parser, Subcommand};
use directories::ProjectDirs;
use indexer::pool::IndexerWorkerPool;
use migration::Migrator;
use migration::MigratorTrait;
use rocksdb::{DBCompressionType, IteratorMode, Options, PrefixRange, ReadOptions, DB};
use sea_orm::{ConnectOptions, Database, DatabaseConnection};
use tokio::time::sleep;
use tracing::{debug, info};

use crate::bucket::Bucket;
use crate::identity::IdentityKeeper;

mod bucket;
mod chunkstorage;
mod datastream_storage;
mod directory_watcher;
mod identity;
mod indexer;
pub(crate) mod object_storage;
mod p2p;
mod rdb;
mod snapshot_storage;
mod sqlentity;

async fn make_snapshot(db: Arc<DB>, rdb: Arc<DatabaseConnection>) {
    let root = Path::new(".");

    let indexer_pool = IndexerWorkerPool::new().start();

    let bucket = Bucket::create(|ctx| {
        Bucket::new(
            db.clone(),
            rdb.clone(),
            root.to_path_buf(),
            indexer_pool.clone().downgrade(),
            ctx.address().downgrade(),
        ).await
    });

    bucket.do_send(ReindexAll {});

    sleep(Duration::from_secs(60 * 10)).await;
}

#[derive(Debug, Parser)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Debug, Subcommand)]
enum Commands {
    ListDatastreams,
    ListSnapshots,
    MakeSnapshot,
    #[command(subcommand)]
    Identity(Identity),
    Run,
}

#[derive(Debug, Subcommand)]
enum Identity {
    Generate {
        #[arg(short, long, default_value_t = false)]
        force: bool,
    },
    PrintPeerId,
}

#[actix_rt::main]
async fn main() {
    tracing_subscriber::fmt::init();

    let dirs = ProjectDirs::from("com", "Librevault", "Librevault Client").unwrap();
    fs::create_dir_all(dirs.config_dir()).unwrap();

    info!("Config base path: {:?}", dirs.config_dir());
    let identity_path = dirs.config_dir().join("identity.bin");
    info!("Identity path: {identity_path:?}");
    let rocksdb_path = dirs.config_dir().join("rocksdb");
    info!("RocksDB database path: {rocksdb_path:?}");
    let sqlite_path = dirs.config_dir().join("data.db3");
    info!("SQLite database path: {sqlite_path:?}");

    let args = Cli::parse();

    let db = {
        let mut options = Options::default();
        options.set_compression_type(DBCompressionType::Zstd);
        options.create_if_missing(true);
        DB::open(&options, rocksdb_path).unwrap()
    };

    let rdb = {
        let _ = fs::OpenOptions::new()
            .append(true)
            .create(true)
            .open(&sqlite_path);

        let url = format!("sqlite:{}", sqlite_path.to_str().unwrap());
        debug!("Opening SQLite database: {}", url);

        let mut options = ConnectOptions::new(url);
        options.sqlx_logging(false);

        let db = Database::connect(options).await.unwrap();

        Migrator::status(&db).await.unwrap();
        Migrator::up(&db, None).await.unwrap();
        db
    };

    match args.command {
        Commands::ListDatastreams => {
            let mut opts = ReadOptions::default();
            opts.set_prefix_same_as_start(true);
            opts.set_iterate_range(PrefixRange("bucket.datastream:"));
            let iter = db.iterator_opt(
                IteratorMode::From("bucket.datastream:".as_bytes(), rocksdb::Direction::Forward),
                opts,
            );
            for i in iter {
                let v = i.unwrap().0;
                // let x = Snapshot::decode(&*v).unwrap();
                println!("{:?}", String::from_utf8(v.into_vec()));
            }
        }
        Commands::ListSnapshots => {
            let mut opts = ReadOptions::default();
            opts.set_prefix_same_as_start(true);
            opts.set_iterate_range(PrefixRange("bucket.snapshot:"));
            let iter = db.iterator_opt(
                IteratorMode::From("bucket.snapshot:".as_bytes(), rocksdb::Direction::Forward),
                opts,
            );
            for i in iter {
                // let v = i.unwrap().1;
                let v = i.unwrap().0;
                // let x = Snapshot::decode(&*v).unwrap();
                println!("{:?}", String::from_utf8(v.into_vec()));
            }
        }
        Commands::MakeSnapshot => {
            make_snapshot(Arc::new(db), Arc::new(rdb)).await;
        }
        Commands::Identity(Identity::Generate { force }) => {
            if identity_path.exists() && !force {
                panic!("Identity already exists in {identity_path:?}");
            };

            let keeper = IdentityKeeper::new(&identity_path);
            let _ = keeper.generate();
        }
        Commands::Identity(Identity::PrintPeerId) => {
            let keeper = IdentityKeeper::new(&identity_path);
            let keypair = keeper.try_get().unwrap();

            println!("{}", keypair.public().to_peer_id());
        }
        Commands::Run => {
            let keeper = IdentityKeeper::new(&identity_path);
            let id_keys = keeper.get();

            let p2p = p2p::P2PNetwork::new(id_keys).start();

            sleep(Duration::from_secs(60 * 10)).await;
        }
    }
}
