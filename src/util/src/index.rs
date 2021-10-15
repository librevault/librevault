use rusqlite::{Connection, Result, Params, named_params, OptionalExtension};
use serde::{Deserialize, Deserializer, Serialize};
use serde::Serializer;
use std::fmt::{Display, Formatter};
use std::path::Path;
use std::sync::{Mutex};
use log::{debug, trace};
use prost::Message;

struct Index {
    conn: Mutex<Connection>,
}

fn as_base64<S: Serializer>(v: &Vec<u8>, s: S) -> Result<S::Ok, S::Error> {
    s.serialize_str(&base64::encode(v))
}

pub fn from_base64<'de, D>(deserializer: D) -> Result<Vec<u8>, D::Error>
    where D: Deserializer<'de>
{
    use serde::de::Error;
    String::deserialize(deserializer)
        .and_then(|string| base64::decode(&string).map_err(|err| Error::custom(err.to_string())))
}

#[derive(Serialize, Deserialize, Clone)]
struct SignedMeta {
    #[serde(serialize_with = "as_base64", deserialize_with = "from_base64")]
    meta: Vec<u8>,
    #[serde(serialize_with = "as_base64", deserialize_with = "from_base64")]
    signature: Vec<u8>,
}

#[derive(Serialize)]
struct Output {
    metas: Vec<SignedMeta>,
}

#[derive(Debug)]
enum IndexError {
    SqlError(rusqlite::Error),
    MetaNotFound,
}

impl From<rusqlite::Error> for IndexError {
    fn from(err: rusqlite::Error) -> IndexError {
        IndexError::SqlError(err)
    }
}

impl Display for IndexError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/librevault.serialization.rs"));
}

impl Index {
    fn migrate(&self) -> rusqlite::Result<usize> {
        debug!("Starting database migration");

        let conn = self.conn.lock().unwrap();

        // Invoke migrations. Potentially destructive!
        (*conn).execute("PRAGMA foreign_keys = ON;", [])?;
        (*conn).execute("CREATE TABLE IF NOT EXISTS meta (path_id BLOB PRIMARY KEY NOT NULL, meta BLOB NOT NULL, signature BLOB NOT NULL, type INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);", [])?;
        (*conn).execute(
            "CREATE INDEX IF NOT EXISTS meta_type_idx ON meta (type);",
            [],
        )?; // For making "COUNT(*) ... WHERE type=x" way faster
        (*conn).execute(
            "CREATE INDEX IF NOT EXISTS meta_not_deleted_idx ON meta(type<>255);",
            [],
        )?; // For faster Index::getExistingMeta
        (*conn).execute("CREATE TABLE IF NOT EXISTS chunk (ct_hash BLOB NOT NULL PRIMARY KEY, size INTEGER NOT NULL, iv BLOB NOT NULL);", [])?;
        (*conn).execute("CREATE TABLE IF NOT EXISTS openfs (ct_hash BLOB NOT NULL REFERENCES chunk (ct_hash) ON DELETE CASCADE ON UPDATE CASCADE, path_id BLOB NOT NULL REFERENCES meta (path_id) ON DELETE CASCADE ON UPDATE CASCADE, [offset] INTEGER NOT NULL, assembled BOOLEAN DEFAULT (0) NOT NULL);", [])?;
        (*conn).execute("CREATE INDEX IF NOT EXISTS openfs_assembled_idx ON openfs (ct_hash, assembled) WHERE assembled = 1;", [])?; // For faster OpenStorage::have_chunk
        (*conn).execute(
            "CREATE INDEX IF NOT EXISTS openfs_path_id_fki ON openfs (path_id);",
            [],
        )?; // For faster AssemblerQueue::assemble_file
        (*conn).execute(
            "CREATE INDEX IF NOT EXISTS openfs_ct_hash_fki ON openfs (ct_hash);",
            [],
        )?; // For faster Index::containingChunk

        Ok(0)
        // db_->exec("CREATE TRIGGER IF NOT EXISTS chunk_deleter AFTER DELETE ON openfs BEGIN DELETE FROM chunk WHERE ct_hash
        // NOT IN (SELECT ct_hash FROM openfs); END;");   // Damn, there are more problems with this trigger than profit from
        // it. Anyway, we can add it anytime later.
    }

    fn new(db_path: &Path) -> Self {
        debug!("Opening database, {:?}", db_path);
        let conn = Connection::open(db_path).unwrap();
        Index {
            conn: Mutex::new(conn),
        }
    }

    fn get_signed_meta<P: Params>(&self, query: &str, params: P) -> Result<Vec<SignedMeta>, IndexError> {
        let conn = self.conn.lock().unwrap();

        let mut meta_stmt = (*conn).prepare(query).unwrap();  // If sql is invalid, panic!
        let meta_iter = meta_stmt.query_map(params, |row| {
            Ok(SignedMeta {
                meta: row.get(0)?,
                signature: row.get(1)?,
            })
        }).ok().ok_or(IndexError::MetaNotFound)?;

        Ok(meta_iter.map(|meta| meta.unwrap()).collect())
    }

    fn get_meta_by_path_id(&self, path_id: &[u8]) -> Result<SignedMeta, IndexError> {
        self.get_signed_meta("SELECT meta, signature FROM meta WHERE path_id=:path_id LIMIT 1", named_params!{":path_id": path_id})?.first().cloned().ok_or(IndexError::MetaNotFound)
    }

    fn get_meta_all(&self) -> Result<Vec<SignedMeta>, IndexError> {
        self.get_signed_meta("SELECT meta, signature FROM meta", [])
    }

    fn get_meta_assembled(&self, assembled: bool) -> Result<Vec<SignedMeta>, IndexError> {
        self.get_signed_meta("SELECT meta, signature FROM meta WHERE (type<>0)=1 AND assembled=:assembled", named_params!{":assembled": assembled})
    }

    fn get_meta_with_chunk(&self, chunk_id: &[u8]) -> Result<Vec<SignedMeta>, IndexError> {
        self.get_signed_meta("SELECT meta.meta, meta.signature FROM meta JOIN openfs ON meta.path_id=openfs.path_id WHERE openfs.ct_hash=:chunk_id", named_params!{":chunk_id": chunk_id})
    }

    fn set_assembled(&self, meta_id: &[u8]) -> Result<(), IndexError> {
        let mut conn = self.conn.lock().unwrap();
        let mut tx = (*conn).transaction()?;

        {
            let sp = tx.savepoint()?;
            sp.execute("UPDATE meta SET assembled=1 WHERE path_id=:meta_id", named_params! {":meta_id": meta_id})?;
            sp.execute("UPDATE openfs SET assembled=1 WHERE path_id=:meta_id", named_params! {":meta_id": meta_id})?;
            sp.commit()?;
        }
        tx.commit()?;
        Ok(())
    }

    fn wipe(&self) -> Result<(), IndexError> {
        let mut conn = self.conn.lock().unwrap();
        let mut tx = (*conn).transaction()?;

        {
            let sp = tx.savepoint()?;
            sp.execute("DELETE FROM meta;", [])?;
            sp.execute("DELETE FROM chunk;", [])?;
            sp.execute("DELETE FROM openfs;", [])?;
            sp.commit()?;
        }
        tx.commit()?;
        Ok(())
    }

    fn is_chunk_assembled(&self, chunk_id: &[u8]) -> Result<bool, IndexError> {
        let conn = self.conn.lock().unwrap();
        let mut meta_stmt = (*conn).prepare("SELECT assembled FROM openfs WHERE ct_hash=:chunk_id AND openfs.assembled=1 LIMIT 1")?;
        Ok(meta_stmt.exists(named_params! {":chunk_id": chunk_id})?)
    }

    fn put_meta(&self, meta: SignedMeta, fully_assembled: bool) -> Result<(), IndexError> {
        let de_meta = proto::Meta::decode(&*meta.meta).unwrap();

        let mut conn = self.conn.lock().unwrap();
        let mut tx = (*conn).transaction()?;
        {
            let sp = tx.savepoint()?;

            let res = sp.execute(
                "INSERT OR REPLACE INTO meta (path_id, meta, signature, type, assembled) VALUES (:path_id, :meta, :signature, :type, :assembled);", named_params! {
                ":path_id": de_meta.path_id,
                ":meta": meta.meta,
                ":signature": meta.signature,
                ":type": de_meta.meta_type,
                ":assembled": fully_assembled
            })?;

            if de_meta.type_specific_metadata.is_some() {
                if let proto::meta::TypeSpecificMetadata::FileMetadata(tsm) = de_meta.type_specific_metadata.unwrap() {
                    trace!("Putting {} chunks", tsm.chunks.len());
                    let mut offset = 0;
                    for chunk in tsm.chunks {
                        sp.execute(
                            "INSERT OR IGNORE INTO chunk (ct_hash, size, iv) VALUES (:ct_hash, :size, :iv);", named_params! {":ct_hash": chunk.ct_hash, ":size": chunk.size, ":iv": chunk.iv}
                        )?;
                        sp.execute(
                            "INSERT OR REPLACE INTO openfs (ct_hash, path_id, [offset], assembled) VALUES (:ct_hash, :path_id, :offset, :assembled);", named_params! {":ct_hash": chunk.ct_hash, ":path_id": de_meta.path_id, ":offset": offset, ":assembled": fully_assembled}
                        )?;
                        offset += chunk.size;
                    }
                }
            }
            sp.commit()?;
        }
        tx.commit()?;

        debug!("Added Meta of {}, t: {:?}, a: {}", hex::encode(de_meta.path_id), de_meta.meta_type, fully_assembled);
        Ok(())
    }
}

fn wrap_result_single(meta: SignedMeta) -> Vec<u8> {
    serde_json::to_vec(&Output { metas: vec![meta] }).unwrap()
}

fn wrap_result_multi(metas: Vec<SignedMeta>) -> Vec<u8> {
    serde_json::to_vec(&Output { metas }).unwrap()
}

impl Index {
    fn c_get_meta_by_path_id(&self, path_id: &[u8]) -> Result<Vec<u8>, IndexError> {
        let meta = self.get_meta_by_path_id(path_id)?;
        Ok(wrap_result_single(meta))
    }

    fn c_get_meta_all(&self) -> Result<Vec<u8>, IndexError> {
        Ok(wrap_result_multi(self.get_meta_all()?))
    }

    fn c_get_meta_assembled(&self, assembled: bool) -> Result<Vec<u8>, IndexError> {
        Ok(wrap_result_multi(self.get_meta_assembled(assembled)?))
    }

    fn c_get_meta_with_chunk(&self, chunk_id: &[u8]) -> Result<Vec<u8>, IndexError> {
        Ok(wrap_result_multi(self.get_meta_with_chunk(chunk_id)?))
    }

    fn c_migrate(&self) -> Result<(), IndexError> {
        let _ = self.migrate()?;
        Ok(())
    }

    fn c_put_meta(&self, signed_meta: &str, fully_assembled: bool) -> Result<(), IndexError> {
        self.put_meta(serde_json::from_str(signed_meta).unwrap(), fully_assembled)
    }
}

fn index_new(db_path: &str) -> Box<Index> {
    Box::new(Index::new(Path::new(db_path)))
}

#[cxx::bridge(namespace = "librevault::bridge")]
mod ffx {
    extern "Rust" {
        type Index;
        fn index_new(db_path: &str) -> Box<Index>;
        fn c_migrate(self: &Index) -> Result<()>;
        fn c_get_meta_by_path_id(self: &Index, path_id: &[u8]) -> Result<Vec<u8>>;
        fn c_get_meta_all(self: &Index) -> Result<Vec<u8>>;
        fn c_get_meta_assembled(self: &Index, assembled: bool) -> Result<Vec<u8>>;
        fn c_get_meta_with_chunk(self: &Index, chunk_id: &[u8]) -> Result<Vec<u8>>;
        fn set_assembled(self: &Index, meta_id: &[u8]) -> Result<()>;
        fn wipe(self: &Index) -> Result<()>;
        fn is_chunk_assembled(self: &Index, chunk_id: &[u8]) -> Result<bool>;
        fn c_put_meta(&self, signed_meta: &str, fully_assembled: bool) -> Result<()>;
    }
}
