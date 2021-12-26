use crate::bucket::Bucket;
use fuse_mt::{
    CallbackResult, CreatedEntry, DirectoryEntry, FileAttr, FileType, FilesystemMT, RequestInfo,
    ResultCreate, ResultData, ResultEmpty, ResultEntry, ResultOpen, ResultReaddir, ResultSlice,
    ResultStatfs, ResultWrite, ResultXattr,
};
use librevault_util::aescbc::decrypt_aes256;
use librevault_util::index::SignedMeta;
use librevault_util::indexer::{make_meta_empty_file, make_tombstone, proto, sign_meta};
use librevault_util::path_normalize::{denormalize, normalize};
use librevault_util::secret::Secret;
use log::debug;
use std::ffi::OsStr;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::time::{Duration, SystemTime};

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

fn meta_size(meta: &proto::Meta) -> u64 {
    if meta.meta_type != 1 {
        if let Some(proto::meta::TypeSpecificMetadata::FileMetadata(fm)) =
            &meta.type_specific_metadata
        {
            return fm.chunks.iter().map(|chunk| chunk.size as u64).sum();
        }
    }
    0
}

fn make_denorm_path_fuse(fuse_path: &Path) -> PathBuf {
    let indexed_path = fuse_path
        .strip_prefix(Path::new("/"))
        .unwrap()
        .to_path_buf();
    debug!(
        "fuse_path: {:?} indexed_path: {:?}",
        fuse_path, indexed_path
    );
    indexed_path
}

fn meta_attr(meta: &proto::Meta) -> Result<FileAttr, libc::c_int> {
    if meta.meta_type == 0 {
        return Err(libc::ENOENT);
    }

    Ok(FileAttr {
        size: meta_size(&meta),
        blocks: 0,
        atime: SystemTime::now(),
        mtime: SystemTime::now(),
        ctime: SystemTime::now(),
        crtime: SystemTime::now(),
        kind: meta_kind(&meta).unwrap(),
        perm: 0,
        nlink: 0,
        uid: 0,
        gid: 0,
        rdev: 0,
        flags: 0,
    })
}

impl LibrevaultFs {
    pub(crate) fn new(bucket: Arc<Bucket>) -> Self {
        LibrevaultFs { bucket }
    }

    fn get_meta_by_path(&self, path: &Path) -> Option<SignedMeta> {
        let denorm_path_fuse = make_denorm_path_fuse(path);

        for signed_meta in self.bucket.index.get_meta_all().unwrap() {
            let meta = signed_meta.parsed_meta();
            let decrypted_path = make_decrypted(meta.path.as_ref().unwrap(), &self.bucket.secret);
            let denorm_path = denormalize(decrypted_path, None).unwrap();
            debug!(
                "path: {:?}, denorm_path: {:?}",
                denorm_path_fuse, denorm_path
            );
            if denorm_path_fuse == denorm_path {
                return Some(signed_meta);
            }
        }
        None
    }
}

const TTL: Duration = Duration::from_secs(60);

impl FilesystemMT for LibrevaultFs {
    fn getattr(&self, _req: RequestInfo, _path: &Path, _fh: Option<u64>) -> ResultEntry {
        if _path == Path::new("/") {
            return Ok((
                TTL,
                FileAttr {
                    size: 0,
                    blocks: 0,
                    atime: SystemTime::now(),
                    mtime: SystemTime::now(),
                    ctime: SystemTime::now(),
                    crtime: SystemTime::now(),
                    kind: FileType::Directory,
                    perm: 0,
                    nlink: 0,
                    uid: 0,
                    gid: 0,
                    rdev: 0,
                    flags: 0,
                },
            ));
        } else if let Some(signed_meta) = self.get_meta_by_path(_path) {
            let meta = signed_meta.parsed_meta();
            return Ok((TTL, meta_attr(&meta)?));
        }
        Err(libc::ENOENT)
    }

    fn unlink(&self, _req: RequestInfo, _parent: &Path, _name: &OsStr) -> ResultEmpty {
        let path = normalize(_parent.join(Path::new(_name)), Path::new(""), true);
        let meta = make_tombstone(path.unwrap().as_slice(), &self.bucket.secret).unwrap();
        let signed_meta = sign_meta(&meta, &self.bucket.secret);
        self.bucket.index.put_meta(&signed_meta, true);
        Ok(())
    }

    fn open(&self, _req: RequestInfo, path: &Path, flags: u32) -> ResultOpen {
        debug!("open: {:?} flags={:#x}", path, flags);
        let fh = 0;
        Ok((fh, flags))
    }

    fn opendir(&self, _req: RequestInfo, _path: &Path, _flags: u32) -> ResultOpen {
        Ok((1, 0))
    }

    fn readdir(&self, _req: RequestInfo, _path: &Path, _fh: u64) -> ResultReaddir {
        let path = _path.strip_prefix("/").unwrap();

        let mut res = vec![];

        for signed_meta in self.bucket.index.get_meta_all().unwrap() {
            let meta = signed_meta.parsed_meta();
            let decrypted_path = make_decrypted(meta.path.as_ref().unwrap(), &self.bucket.secret);
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

    fn create(
        &self,
        _req: RequestInfo,
        _parent: &Path,
        _name: &OsStr,
        _mode: u32,
        _flags: u32,
    ) -> ResultCreate {
        let path = normalize(_parent.join(Path::new(_name)), Path::new(""), true);
        let meta = make_meta_empty_file(path.unwrap().as_slice(), &self.bucket.secret).unwrap();
        let signed_meta = sign_meta(&meta, &self.bucket.secret);
        self.bucket.index.put_meta(&signed_meta, true);
        Ok(CreatedEntry {
            ttl: TTL,
            attr: meta_attr(&meta)?,
            fh: 1666444,
            flags: 0,
        })
    }
}
