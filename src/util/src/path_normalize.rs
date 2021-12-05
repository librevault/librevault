use log::trace;
use path_slash::{PathBufExt, PathExt};
use std::fmt::{Display, Formatter};
use std::path::{Path, PathBuf};
use unicode_normalization::UnicodeNormalization;

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Clone)]
pub enum NormalizationError {
    PrefixError,
    UnicodeError,
}

impl Display for NormalizationError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

pub fn normalize(
    path: impl AsRef<Path>,
    root: impl AsRef<Path>,
    normalize_unicode: bool,
) -> Result<Vec<u8>, NormalizationError> {
    let mut normalized = path
        .as_ref()
        .strip_prefix(root)
        .ok()
        .ok_or(NormalizationError::PrefixError)?
        .to_slash()
        .ok_or(NormalizationError::PrefixError)?;

    if normalize_unicode {
        normalized = normalized.nfc().collect::<String>();
    }

    normalized = String::from(normalized.trim_matches('/'));

    trace!("path: {:?} normalized: {:?}", path.as_ref(), normalized);

    Ok(normalized.into_bytes())
}

pub fn denormalize(
    path: impl Into<Vec<u8>>,
    root: Option<&Path>,
) -> Result<PathBuf, NormalizationError> {
    let denormalized = String::from_utf8(Into::into(path))
        .ok()
        .ok_or(NormalizationError::UnicodeError)?;
    let mut denormalized = PathBuf::from_slash(denormalized);
    if let Some(root) = root {
        denormalized = root.join(denormalized);
    }

    // macos uses a weird NFD-normalized form. It can be achieved using fileSystemRepresentation function

    Ok(denormalized)
}

fn normalize_cxx(
    path: &str,
    root: &str,
    normalize_unicode: bool,
) -> Result<Vec<u8>, NormalizationError> {
    normalize(Path::new(path), Path::new(root), normalize_unicode)
}

fn denormalize_cxx(path: &[u8], root: &str) -> Result<String, NormalizationError> {
    Ok(String::from(
        denormalize(path, Some(Path::new(root)))?.to_str().unwrap(),
    ))
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn normalize_cxx(path: &str, root: &str, normalize_unicode: bool) -> Result<Vec<u8>>;
        fn denormalize_cxx(path: &[u8], root: &str) -> Result<String>;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_no_simple() {
        assert_eq!(
            normalize(Path::new("/home/123/123.txt"), Path::new("/home/123"), true).unwrap(),
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
            )
            .unwrap(),
            b"123.txt".to_vec()
        );
    }

    #[test]
    fn test_no_normalization() {
        let path_nfd = Path::new("/home/123/é.txt");
        let path_nfc = "é.txt".to_string().into_bytes();
        assert_ne!(
            normalize(path_nfd, Path::new("/home/123"), false).unwrap(),
            path_nfc
        );
        assert_eq!(
            normalize(path_nfd, Path::new("/home/123"), true).unwrap(),
            path_nfc
        );
    }

    #[test]
    fn test_de_simple() {
        assert_eq!(
            denormalize(b"123.txt".to_vec(), Some(Path::new("/home/123"))).unwrap(),
            PathBuf::from("/home/123/123.txt")
        );
    }
}
