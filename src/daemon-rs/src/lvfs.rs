use crate::bucket::Bucket;
use fuse_mt::{DirectoryEntry, FileType, FilesystemMT, RequestInfo, ResultEntry, ResultReaddir};
use librevault_util::aescbc::decrypt_aes256;
use librevault_util::indexer::proto;
use librevault_util::path_normalize::denormalize;
use librevault_util::secret::Secret;
use std::path::Path;
use std::sync::Arc;

pub(crate) struct LibrevaultFs {
    bucket: Arc<Bucket>,
}

fn make_decrypted(data: &proto::AesCbc, secret: &Secret) -> Vec<u8> {
    decrypt_aes256(&*data.ct, secret.get_symmetric_key().unwrap(), &*data.iv)
}

fn meta_kind(meta: &proto::Meta) -> Option<FileType> {
    match meta.meta_type {
        1 => Some(FileType::RegularFile),
        2 => Some(FileType::Directory),
        3 => Some(FileType::Symlink),
        _ => None,
    }
}

impl LibrevaultFs {
    pub(crate) fn new(bucket: Arc<Bucket>) -> Self {
        LibrevaultFs { bucket }
    }
}

impl FilesystemMT for LibrevaultFs {
    fn getattr(&self, _req: RequestInfo, _path: &Path, _fh: Option<u64>) -> ResultEntry {
        todo!()
    }

    fn readdir(&self, _req: RequestInfo, _path: &Path, _fh: u64) -> ResultReaddir {
        let path = _path.strip_prefix("/").unwrap();

        let mut res = vec![];

        for signed_meta in self.bucket.index.get_meta_all().unwrap() {
            let meta = signed_meta.parsed_meta();
            let decrypted_path = make_decrypted(&meta.path.as_ref().unwrap(), &self.bucket.secret);
            let denorm_path = denormalize(&*decrypted_path, None).unwrap();
            if let Ok(unprefixed_path) = denorm_path.strip_prefix(path) {
                let next_component = unprefixed_path.components().next().unwrap().as_os_str();

                let kind = match meta_kind(&meta) {
                    Some(kind) => kind,
                    _ => continue,
                };

                res.push(DirectoryEntry {
                    name: next_component.into(),
                    kind,
                });
            }
        }
        Ok(res)
    }
}
