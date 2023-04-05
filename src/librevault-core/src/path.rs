use path_slash::{PathBufExt, PathExt};
use std::fmt::{Display, Formatter};
use std::path::{Path, PathBuf, StripPrefixError};
use std::str::Utf8Error;
use unicode_normalization::UnicodeNormalization;

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Clone)]
pub enum NormalizationError {
    PrefixError,
    UnicodeError,
}

impl From<Utf8Error> for NormalizationError {
    fn from(_: Utf8Error) -> Self {
        NormalizationError::UnicodeError
    }
}

impl From<StripPrefixError> for NormalizationError {
    fn from(_: StripPrefixError) -> Self {
        NormalizationError::PrefixError
    }
}

impl Display for NormalizationError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{self:?}")
    }
}

#[derive(Clone)]
pub struct PointerPath(Vec<u8>);

impl PointerPath {
    pub fn from_path(
        path: impl AsRef<Path>,
        root: impl AsRef<Path>,
        normalize_unicode: bool,
    ) -> Result<Self, NormalizationError> {
        let mut normalized = path
            .as_ref()
            .strip_prefix(root)?
            .to_slash()
            .ok_or(NormalizationError::PrefixError)?
            .to_string();

        if normalize_unicode {
            normalized = normalized.nfc().collect::<String>();
        }

        normalized = String::from(normalized.trim_matches('/'));

        // trace!("path: {:?} normalized: {:?}", path.as_ref(), normalized);

        Ok(Self(normalized.into_bytes()))
    }

    pub fn as_path(&self, root: Option<&Path>) -> Result<PathBuf, NormalizationError> {
        let normalized_str = std::str::from_utf8(&self.0)?;

        let mut denormalized = PathBuf::from_slash(normalized_str);
        if let Some(root) = root {
            denormalized = root.join(denormalized);
        }

        // macos uses a weird NFD-normalized form. It can be achieved using fileSystemRepresentation function

        // trace!("normalized: {normalized_str:?} denormalized: {denormalized:?}");

        Ok(denormalized)
    }

    pub fn into_vec(self) -> Vec<u8> {
        self.0
    }

    pub fn hex(&self) -> String {
        hex::encode(&self.0)
    }
}

impl From<Vec<u8>> for PointerPath {
    fn from(value: Vec<u8>) -> Self {
        PointerPath(value)
    }
}
