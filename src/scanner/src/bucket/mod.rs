use std::path::PathBuf;
use std::sync::Arc;

use actix::{Actor, Addr, Handler, Message as ActixMessage, WeakAddr};
use libp2p_identity::PeerId;
use librevault_core::indexer::reference::ReferenceMaker;
use prost::Message;
use ractor::Actor as RactorActor;
use ractor::ActorRef;
use rocksdb::DB;
use sea_orm::DatabaseConnection;

use crate::chunkstorage::materialized::MaterializedFolder;
use crate::datastream_storage::DataStreamStorage;
use crate::directory_watcher;
use crate::directory_watcher::actor::DirectoryWatcherActor;
use crate::directory_watcher::DirectoryWatcher;
use crate::indexer::bucket::BucketIndexer;
use crate::indexer::pool::IndexerWorkerPool;
use crate::indexer::{IndexingTask, Policy};
use crate::object_storage::ObjectMetadataStorage;
use crate::rdb::RocksDBObjectCRUD;
use crate::snapshot_storage::SnapshotStorage;

pub mod actor;

type BucketId = PeerId;

pub struct Bucket {
    snapstore: Arc<SnapshotStorage>,
    dss: Arc<DataStreamStorage>,
    objstore: Addr<ObjectMetadataStorage>,

    watcher: ActorRef<DirectoryWatcher>,
    indexer_pool: WeakAddr<IndexerWorkerPool>,

    materialized_storage: Addr<MaterializedFolder>,
    indexer: Addr<BucketIndexer>,

    root: PathBuf,
}

impl Bucket {
    pub async fn new(
        db: Arc<DB>,
        rdb: Arc<DatabaseConnection>,
        root: PathBuf,
        indexer_pool: WeakAddr<IndexerWorkerPool>,
        addr: WeakAddr<Bucket>,
    ) -> Self {
        let (watcher, _) = RactorActor::spawn(
            None,
            DirectoryWatcherActor,
            directory_watcher::actor::Arguments { root: root.clone() },
        )
        .await
        .unwrap();

        let snapstore = Arc::new(SnapshotStorage::new(db.clone()));
        let dss = Arc::new(DataStreamStorage::new(db.clone()));
        let objstore = ObjectMetadataStorage::new(db.clone()).start();

        let materialized_storage = MaterializedFolder::new(
            &root,
            db.clone(),
            rdb.clone(),
            dss.clone(),
            objstore.clone(),
        )
        .start();

        let indexer = BucketIndexer::new(
            indexer_pool.clone().upgrade().unwrap(),
            materialized_storage.clone(),
            dss.clone(),
            addr.clone().recipient(),
        )
        .start();

        Self {
            snapstore,
            dss,
            objstore,

            watcher,
            indexer_pool,

            materialized_storage,
            indexer,

            root,
        }
    }
}
