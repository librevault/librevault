use std::io::SeekFrom;
use std::path::{Path, PathBuf};
use std::sync::Arc;

use librevault_core::path::PointerPath;
use librevault_core::proto::object_metadata::ObjectType;
use librevault_core::proto::{ChunkMetadata, DataStream, ReferenceHash};
use rocksdb::DB;
use sea_orm::{
    ActiveModelTrait, ColumnTrait, ConnectionTrait, DatabaseConnection, EntityTrait, QueryFilter,
    Set,
};
use tokio::io::{AsyncReadExt, AsyncSeekExt};
use tracing::debug;

use crate::chunkstorage::{ChunkLocationHint, ChunkProvider, ChunkStorageError, QueryResult};
use crate::datastream_storage::DataStreamStorage;
use crate::object_storage::ObjectMetadataStorage;
use crate::rdb::RocksDBObjectCRUD;
use crate::sqlentity::materialized;

pub struct MaterializedFolder {
    root: PathBuf,
    db: Arc<DB>,
    rdb: Arc<DatabaseConnection>,
    dss: Arc<DataStreamStorage>,
    objstore: Arc<ObjectMetadataStorage>,
}

impl MaterializedFolder {
    pub fn new(
        root: &Path,
        db: Arc<DB>,
        rdb: Arc<DatabaseConnection>,
        dss: Arc<DataStreamStorage>,
        objstore: Arc<ObjectMetadataStorage>,
    ) -> Self {
        Self {
            root: root.into(),
            db,
            rdb,
            dss,
            objstore,
        }
    }

    pub fn root(&self) -> &Path {
        &self.root
    }

    async fn assume_materialized_chunk<C: ConnectionTrait>(
        &self,
        c: &C,
        chunk_md_refh: &ReferenceHash,
        chunk_md: &ChunkMetadata,
        path: &PointerPath,
        offset: u64,
    ) {
        let path = path.clone().into_vec();
        let offset = offset.try_into().unwrap();
        let chunk_md_hash: Vec<u8> = chunk_md_refh.as_ref().into();
        let plaintext_hash: Vec<u8> = chunk_md.chunk_pt.as_ref().unwrap().as_ref().into();
        let bucket = vec![];

        match materialized::Entity::find()
            .filter(materialized::Column::Path.eq(path.clone()))
            .filter(materialized::Column::Offset.eq(offset))
            .filter(materialized::Column::ChunkMdHash.eq(chunk_md_hash.clone()))
            .filter(materialized::Column::PlaintextHash.eq(plaintext_hash.clone()))
            .filter(materialized::Column::Bucket.eq(bucket.clone()))
            .one(c)
            .await
            .unwrap()
        {
            None => {
                let row = materialized::ActiveModel {
                    path: Set(path),
                    offset: Set(offset),
                    chunk_md_hash: Set(chunk_md_hash),
                    plaintext_hash: Set(plaintext_hash),
                    bucket: Set(bucket),
                    ..Default::default()
                };
                row.save(&*self.rdb).await.unwrap();
            }
            Some(_) => debug!("Found materialized chunk"),
        }
    }

    pub async fn get_chunk_md_by_plaintext_hash(
        &self,
        chunk_pt_refh: &ReferenceHash,
    ) -> Option<ChunkMetadata> {
        let entry = materialized::Entity::find()
            .filter(materialized::Column::PlaintextHash.eq(chunk_pt_refh.clone().as_ref()))
            .one(&*self.rdb)
            .await
            .unwrap()?;

        self.dss.get_chunk_md(&ReferenceHash {
            multihash: entry.chunk_md_hash,
        })
    }

    pub async fn assume_materialized(&self, object_md: &ReferenceHash) {
        // let txn = self.rdb.begin().await.unwrap();

        let object_md = self.objstore.get_entity(object_md).unwrap();

        let object_path: PointerPath = object_md.path.into();

        if object_md.r#type == ObjectType::File as i32 {
            let datastream_ref = object_md.stream.unwrap();
            let datastream: DataStream = self.dss.get_entity(&datastream_ref).unwrap();

            let all_chunks = self.dss.get_chunk_md_bulk(&datastream.chunk_md);

            // println!("{all_chunks:?}");

            let mut offset = 0;
            for (chunk_md_refh, chunk_md) in all_chunks {
                let Some(chunk_md) = chunk_md else {
                    continue
                };
                self.assume_materialized_chunk(
                    &*self.rdb,
                    &chunk_md_refh,
                    &chunk_md,
                    &object_path,
                    offset,
                )
                .await;
                offset += chunk_md.size as u64;
            }
        }

        // txn.commit().await.unwrap();
    }

    async fn get_from_hint(
        &self,
        hint: ChunkLocationHint,
    ) -> Result<QueryResult, ChunkStorageError> {
        let ChunkLocationHint::MaterializedLocation { path, offset, length} = &hint else {
            return Err(ChunkStorageError::ChunkNotFound)
        };

        let mut f = match tokio::fs::File::open(path).await {
            Ok(f) => f,
            Err(_) => return Err(ChunkStorageError::ChunkNotFound),
        };

        if f.seek(SeekFrom::Start(*offset)).await.is_err() {
            return Err(ChunkStorageError::ChunkNotFound);
        };

        let mut chunk = vec![0; *length];
        if f.read_exact(&mut chunk).await.is_err() {
            return Err(ChunkStorageError::ChunkNotFound);
        }

        return Ok(QueryResult { chunk, hint });
    }
}

#[async_trait::async_trait]
impl ChunkProvider for MaterializedFolder {
    async fn get(
        &self,
        hash: &ReferenceHash,
        hint: Option<ChunkLocationHint>,
    ) -> Result<QueryResult, ChunkStorageError> {
        let files = materialized::Entity::find()
            .filter(materialized::Column::ChunkMdHash.eq(hash.as_ref()))
            .all(&*self.rdb)
            .await
            .unwrap();

        let chunk_md = self.dss.get_chunk_md(hash).unwrap();

        for file in files {
            let pointer_path = PointerPath::from(file.path);

            let path = pointer_path.as_path(Some(&self.root)).unwrap();

            let hint = ChunkLocationHint::MaterializedLocation {
                path,
                offset: file.offset.try_into().unwrap(),
                length: chunk_md.size as usize,
            };

            return self.get_from_hint(hint).await;
        }

        Err(ChunkStorageError::ChunkNotFound)
    }
}
