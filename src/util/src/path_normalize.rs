use path_slash::{PathBufExt, PathExt};
use std::path::{Path, PathBuf};
use unicode_normalization::UnicodeNormalization;

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Clone)]
pub enum NormalizationError {
    UnicodeError,
}

fn normalize(path: &Path, root: &Path, normalize_unicode: bool) -> Vec<u8> {
    let mut normalized = path.strip_prefix(root).unwrap().to_slash().unwrap();

    if normalize_unicode {
        normalized = normalized.nfc().collect::<String>();
    }

    if normalized.ends_with("/") {
        normalized.pop();
    }

    log::trace!("path: \"{:?}\" normalized: \"{:?}\"", path, normalized);

    normalized.into_bytes()
}

fn denormalize(path: Vec<u8>, root: &Path) -> Result<PathBuf, NormalizationError> {
    let denormalized = String::from_utf8(path)
        .ok()
        .ok_or(NormalizationError::UnicodeError)?;
    let denormalized = PathBuf::from_slash(denormalized);
    let denormalized = root.join(denormalized);

    // macos uses a weird NFD-normalized form. It can be achieved using fileSystemRepresentation function

    Ok(denormalized)
}

mod ffi {
    use super::*;
    use crate::ffi::FfiConstBuffer;
    use std::ffi::CStr;
    use std::os::raw::c_char;

    #[no_mangle]
    pub extern "C" fn path_normalize(
        path: *const c_char,
        root: *const c_char,
        normalize_unicode: bool,
    ) -> FfiConstBuffer {
        let (path, root) = unsafe {
            (
                CStr::from_ptr(path).to_str().unwrap(),
                CStr::from_ptr(root).to_str().unwrap(),
            )
        };
        FfiConstBuffer::from_vec(&normalize(
            &PathBuf::from(path),
            &PathBuf::from(root),
            normalize_unicode,
        ))
    }

    #[no_mangle]
    pub extern "C" fn path_denormalize(path: *const c_char, root: *const c_char) -> FfiConstBuffer {
        let (path, root) = unsafe {
            (
                CStr::from_ptr(path).to_str().unwrap(),
                CStr::from_ptr(root).to_str().unwrap(),
            )
        };
        FfiConstBuffer::from_slice(
            &denormalize(path.to_string().into_bytes(), &PathBuf::from(root))
                .unwrap()
                .to_str()
                .unwrap()
                .as_bytes(),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_no_simple() {
        assert_eq!(
            normalize(Path::new("/home/123/123.txt"), Path::new("/home/123"), true),
            b"123.txt".to_vec()
        );
    }

    #[test]
    fn test_no_trailing_slash() {
        assert_eq!(
            normalize(
                Path::new("/home/123/123.txt/"),
                Path::new("/home/123"),
                true
            ),
            b"123.txt".to_vec()
        );
    }

    #[test]
    fn test_no_normalization() {
        let path_nfd = Path::new("/home/123/é.txt");
        let path_nfc = "é.txt".to_string().into_bytes();
        assert_ne!(normalize(path_nfd, Path::new("/home/123"), false), path_nfc);
        assert_eq!(normalize(path_nfd, Path::new("/home/123"), true), path_nfc);
    }

    #[test]
    fn test_de_simple() {
        assert_eq!(
            denormalize(b"123.txt".to_vec(), Path::new("/home/123")).unwrap(),
            PathBuf::from("/home/123/123.txt")
        );
    }
}
