use log::debug;
use serde::__private::fmt::Display;
use std::fmt::Formatter;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::{fs, io};

#[derive(Debug)]
struct EncryptedStorage {
    root: PathBuf,
}

#[derive(Debug)]
enum StorageError {
    ChunkNotFound,
    IoError { error: io::Error },
}

impl Display for StorageError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

fn make_chunk_path(root: &Path, chunk_id: &[u8]) -> PathBuf {
    let mut filename = base32::encode(base32::Alphabet::RFC4648 { padding: true }, chunk_id);
    filename += ".lvchk";

    root.join(filename)
}

impl EncryptedStorage {
    fn new(root: &Path) -> Self {
        debug!("Creating EncryptedStorage with path: {:?}", root);
        EncryptedStorage {
            root: root.to_path_buf(),
        }
    }

    fn have_chunk(&self, chunk_id: &[u8]) -> bool {
        make_chunk_path(&*self.root, chunk_id).exists()
    }

    fn get_chunk(&self, chunk_id: &[u8]) -> Result<Vec<u8>, StorageError> {
        let mut f = match fs::File::open(make_chunk_path(&*self.root, chunk_id)) {
            Ok(f) => f,
            Err(_) => return Err(StorageError::ChunkNotFound),
        };
        let mut data = vec![];
        match f.read_to_end(&mut data) {
            Ok(_) => Ok(data),
            Err(err) => Err(StorageError::IoError { error: err }),
        }
    }

    fn put_chunk(&self, chunk_id: &[u8], data: &[u8]) {
        let mut f = fs::File::create(make_chunk_path(&*self.root, chunk_id)).unwrap();
        let _ = f.write_all(data);
        debug!("Chunk {} pushed into EncStorage", hex::encode(chunk_id));
    }

    fn remove_chunk(&self, chunk_id: &[u8]) {
        let _ = fs::remove_file(make_chunk_path(&*self.root, chunk_id));
        debug!("Chunk {} removed from EncStorage", hex::encode(chunk_id));
    }
}

fn encryptedstorage_new(root: &str) -> Box<EncryptedStorage> {
    Box::new(EncryptedStorage::new(Path::new(root)))
}

#[cxx::bridge(namespace = "librevault::bridge")]
mod ffi {
    extern "Rust" {
        type EncryptedStorage;
        fn encryptedstorage_new(root: &str) -> Box<EncryptedStorage>;
        fn have_chunk(self: &EncryptedStorage, chunk_id: &[u8]) -> bool;
        fn get_chunk(self: &EncryptedStorage, chunk_id: &[u8]) -> Result<Vec<u8>>;
        fn put_chunk(self: &EncryptedStorage, chunk_id: &[u8], data: &[u8]);
        fn remove_chunk(self: &EncryptedStorage, chunk_id: &[u8]);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip() {
        let temp = tempfile::tempdir().unwrap();
        let storage = EncryptedStorage::new(temp.path());
        let chunk_id = b"12345";
        assert!(!storage.have_chunk(chunk_id));
        let data = b"pdkfspfknwpef";
        storage.put_chunk(chunk_id, data);
        assert!(storage.have_chunk(chunk_id));

        assert_eq!(storage.get_chunk(chunk_id).unwrap(), data);

        storage.remove_chunk(chunk_id);
        assert!(!storage.have_chunk(chunk_id));
    }
}
