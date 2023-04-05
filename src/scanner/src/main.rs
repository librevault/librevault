use std::io::Read;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::Duration;

use actix::Actor;
use chunkstorage::materialized::MaterializedFolder;
use clap::{Args, Parser, Subcommand, ValueEnum};
use librevault_core::indexer::chunk_metadata::ChunkMetadataIndexer;
use librevault_core::indexer::object_metadata::ObjectMetadataIndexer;
use librevault_core::indexer::reference::{ReferenceExt, ReferenceMaker};
use librevault_core::proto::{ChunkMetadata, DataStream, ObjectMetadata, ReferenceHash};
use migration::Migrator;
use migration::MigratorTrait;
use multihash::MultihashDigest;
use prost::Message;
use rocksdb::{DBCompressionType, IteratorMode, Options, PrefixRange, ReadOptions, DB};
use sea_orm::{ConnectOptions, Database, DatabaseConnection};
use tokio::time::sleep;
use walkdir::WalkDir;

use crate::datastream_storage::DataStreamStorage;
use crate::directory_watcher::DirectoryWatcher;
use crate::indexer::bucket::BucketIndexer;
use crate::indexer::{GlobalIndexerActor, IndexingTask, Policy};
use crate::object_storage::ObjectMetadataStorage;
use crate::snapshot_storage::SnapshotStorage;

mod chunkstorage;
mod datastream_storage;
mod directory_watcher;
mod indexer;
mod object_storage;
mod rdb;
mod snapshot_storage;
mod sqlentity;

#[derive(Default)]
struct IndexResult {
    object: (ObjectMetadata, Vec<u8>, ReferenceHash),
    datastreams: Vec<(DataStream, Vec<u8>, ReferenceHash)>,
    blocks: Vec<(ChunkMetadata, Vec<u8>, ReferenceHash)>,
}

async fn make_snapshot(db: Arc<DB>, rdb: Arc<DatabaseConnection>) {
    let root = Path::new(".");

    let dss = Arc::new(DataStreamStorage::new(db.clone()));
    let objstore = Arc::new(ObjectMetadataStorage::new(db.clone()));
    let snapstore = Arc::new(SnapshotStorage::new(db.clone()));
    let materialized_storage = Arc::new(MaterializedFolder::new(
        root,
        db.clone(),
        rdb.clone(),
        dss.clone(),
        objstore.clone(),
    ));
    let watcher = DirectoryWatcher::new(root).start();

    let indexer = GlobalIndexerActor::new().start();
    let bucket_indexer = BucketIndexer::new(indexer, materialized_storage.clone()).start();

    for entry in WalkDir::new(root).min_depth(1).follow_links(false) {
        let de = entry.unwrap();

        bucket_indexer.do_send(IndexingTask {
            policy: Policy::Orphan,
            root: root.into(),
            path: de.path().to_path_buf(),
        });
    }

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
