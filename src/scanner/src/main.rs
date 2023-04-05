use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::Duration;

use actix::{Actor, AsyncContext};
use clap::{Parser, Subcommand};
use indexer::pool::IndexerWorkerPool;
use migration::Migrator;
use migration::MigratorTrait;
use rocksdb::{DBCompressionType, IteratorMode, Options, PrefixRange, ReadOptions, DB};
use sea_orm::{ConnectOptions, Database, DatabaseConnection};
use tokio::time::sleep;

use crate::bucket::{Bucket, ReindexAll};

mod bucket;
mod chunkstorage;
mod datastream_storage;
mod directory_watcher;
mod indexer;
mod object_storage;
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
        )
    });

    bucket.do_send(ReindexAll {});

    sleep(Duration::from_secs(60 * 10)).await;
}

#[derive(Debug, Parser)]
struct Cli {
    #[command(subcommand)]
    command: Commands,

    bucket: PathBuf,
    sqlite: PathBuf,
}

#[derive(Debug, Subcommand)]
enum Commands {
    ListDatastreams,
    ListSnapshots,
    MakeSnapshot,
}

#[actix_rt::main]
async fn main() {
    tracing_subscriber::fmt::init();

    let args = Cli::parse();

    let db = {
        let mut options = Options::default();
        options.set_compression_type(DBCompressionType::Zstd);
        options.create_if_missing(true);
        DB::open(&options, args.bucket).unwrap()
    };

    let rdb = {
        let _ = std::fs::OpenOptions::new()
            .append(true)
            .create(true)
            .open(&args.sqlite);

        let mut options = ConnectOptions::new(format!("sqlite:{}", args.sqlite.to_str().unwrap()));
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
    }
}
