use std::fmt::{Display, Formatter};
use std::os::raw::c_char;
use std::str::FromStr;

use ed25519::signature::Signature;
use ed25519_dalek_fiat::{Keypair, PublicKey, SecretKey, Signer, Verifier, PUBLIC_KEY_LENGTH};
use lazy_static::lazy_static;
use luhn::Luhn;
use rand::rngs::OsRng;
use sha3::{Digest, Sha3_256};

lazy_static! {
    static ref LUHNGENERATOR: Luhn =
        Luhn::new("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz").unwrap();
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Clone)]
pub enum SecretError {
    ParseFailed,
    InvalidVersion,
    ChecksumError {
        secret: String,
        expected: char,
        computed: char,
        payload: String,
    },
    Base58Error,
    CryptoError,
    InvalidType,
    DeriveError,
    InvalidSignatureFormat,
}

impl Display for SecretError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Secret(\"{}\")", self.to_string())
    }
}

#[derive(Debug)]
pub enum OpaqueSecret {
    Signer {
        private_key: SecretKey,
        symmetric_key: Vec<u8>,
        public_key: PublicKey,
    },
    Decryptor {
        symmetric_key: Vec<u8>,
        public_key: PublicKey,
    },
    Verifier {
        public_key: PublicKey,
    },
}

const CURRENT_VERSION: char = '2';

impl OpaqueSecret {
    fn new() -> Self {
        let mut rng = OsRng::default();
        let keypair: Keypair = Keypair::generate(&mut rng);

        OpaqueSecret::Signer {
            symmetric_key: Sha3_256::digest(&keypair.secret.to_bytes()).to_vec(),
            public_key: keypair.public,
            private_key: keypair.secret,
        }
    }

    fn get_private_key(&self) -> Result<&SecretKey, SecretError> {
        match self {
            OpaqueSecret::Signer { private_key, .. } => Ok(&private_key),
            _ => Err(SecretError::DeriveError),
        }
    }

    pub fn get_symmetric_key(&self) -> Result<&[u8], SecretError> {
        match self {
            OpaqueSecret::Signer { symmetric_key, .. }
            | OpaqueSecret::Decryptor { symmetric_key, .. } => Ok(&symmetric_key),
            _ => Err(SecretError::DeriveError),
        }
    }

    fn get_public_key(&self) -> Result<&PublicKey, SecretError> {
        match self {
            OpaqueSecret::Signer { public_key, .. }
            | OpaqueSecret::Decryptor { public_key, .. }
            | OpaqueSecret::Verifier { public_key, .. } => Ok(&public_key),
        }
    }

    fn derive(&self, ty: char) -> Result<Self, SecretError> {
        match ty {
            'A' => Ok(OpaqueSecret::Signer {
                private_key: SecretKey::from_bytes(&self.get_private_key()?.to_bytes()).unwrap(),
                symmetric_key: self.get_symmetric_key()?.to_vec(),
                public_key: self.get_public_key()?.clone(),
            }),
            'B' => Ok(OpaqueSecret::Decryptor {
                symmetric_key: self.get_symmetric_key()?.to_vec(),
                public_key: self.get_public_key()?.clone(),
            }),
            'C' => Ok(OpaqueSecret::Verifier {
                public_key: self.get_public_key()?.clone(),
            }),
            _ => Err(SecretError::DeriveError),
        }
    }

    fn sign(&self, message: &[u8]) -> Result<Vec<u8>, SecretError> {
        let keypair = Keypair::from_bytes(
            &([
                self.get_private_key()?.to_bytes(),
                self.get_public_key()?.to_bytes(),
            ]
            .concat()),
        )
        .unwrap();
        Ok(keypair.sign(message).to_bytes().to_vec())
    }

    fn verify(&self, message: &[u8], signature: &[u8]) -> Result<bool, SecretError> {
        let signature = ed25519::Signature::from_bytes(signature)
            .ok()
            .ok_or(SecretError::InvalidSignatureFormat)?;
        Ok(self.get_public_key()?.verify(message, &signature).is_ok())
    }
}

impl FromStr for OpaqueSecret {
    type Err = SecretError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let secret_type = s.chars().nth(0).ok_or(SecretError::ParseFailed)?;
        let secret_version = s.chars().nth(1).ok_or(SecretError::ParseFailed)?;
        let secret_payload = &s[2..s.len() - 1];
        let secret_luhn = &s.chars().nth_back(0).ok_or(SecretError::ParseFailed)?;

        if secret_version != CURRENT_VERSION {
            return Err(SecretError::InvalidVersion);
        }

        let decrypted_payload = bs58::decode(secret_payload)
            .into_vec()
            .ok()
            .ok_or(SecretError::Base58Error)?;
        let computed_luhn = LUHNGENERATOR.generate(&secret_payload).unwrap();
        if computed_luhn != *secret_luhn {
            return Err(SecretError::ChecksumError {
                secret: s.to_string(),
                computed: computed_luhn,
                expected: *secret_luhn,
                payload: secret_payload.to_string(),
            });
        }

        match secret_type {
            'A' => Ok({
                let private_key = SecretKey::from_bytes(&decrypted_payload)
                    .ok()
                    .ok_or(SecretError::CryptoError)?;
                OpaqueSecret::Signer {
                    public_key: PublicKey::from(&private_key),
                    symmetric_key: Sha3_256::digest(&private_key.to_bytes()).to_vec(),
                    private_key,
                }
            }),
            'B' => Ok({
                let public_key =
                    PublicKey::from_bytes(secret_payload[0..PUBLIC_KEY_LENGTH].as_bytes())
                        .ok()
                        .ok_or(SecretError::CryptoError)?;
                let symmetric_key = secret_payload[PUBLIC_KEY_LENGTH..Sha3_256::output_size()]
                    .as_bytes()
                    .to_vec();
                OpaqueSecret::Decryptor {
                    symmetric_key,
                    public_key,
                }
            }),
            'C' => Ok({
                OpaqueSecret::Verifier {
                    public_key: PublicKey::from_bytes(&decrypted_payload)
                        .ok()
                        .ok_or(SecretError::CryptoError)?,
                }
            }),
            _ => Err(SecretError::InvalidType),
        }
    }
}

impl ToString for OpaqueSecret {
    fn to_string(&self) -> String {
        let (ty, payload) = match self {
            OpaqueSecret::Signer { private_key, .. } => ('A', private_key.to_bytes().to_vec()),
            OpaqueSecret::Decryptor {
                public_key,
                symmetric_key,
            } => ('B', {
                [&public_key.to_bytes()[..], symmetric_key.as_slice()].concat()
            }),
            OpaqueSecret::Verifier { public_key } => ('C', public_key.to_bytes().to_vec()),
        };
        let encoded_payload = bs58::encode(payload).into_string();
        let luhn = LUHNGENERATOR.generate(&encoded_payload).unwrap();
        format!("{}{}{}{}", ty, CURRENT_VERSION, encoded_payload, luhn)
    }
}

impl Clone for OpaqueSecret {
    fn clone(&self) -> Self {
        match self {
            OpaqueSecret::Signer {
                private_key,
                public_key,
                symmetric_key,
            } => OpaqueSecret::Signer {
                private_key: SecretKey::from_bytes(&private_key.to_bytes()[..]).unwrap(),
                public_key: public_key.clone(),
                symmetric_key: symmetric_key.clone(),
            },
            OpaqueSecret::Decryptor {
                public_key,
                symmetric_key,
            } => OpaqueSecret::Decryptor {
                public_key: public_key.clone(),
                symmetric_key: symmetric_key.clone(),
            },
            OpaqueSecret::Verifier { public_key } => OpaqueSecret::Verifier {
                public_key: public_key.clone(),
            },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip() {
        let secret = OpaqueSecret::new();
        let serialized = secret.to_string();
        let parsed = OpaqueSecret::from_str(serialized.as_str()).unwrap();
        let serialized_once_more = parsed.to_string();
        assert_eq!(serialized, serialized_once_more);
    }

    #[test]
    fn test_clone() {
        let secret1 = OpaqueSecret::new();
        let secret2 = secret1.clone();
        assert_eq!(secret1.to_string(), secret2.to_string());
    }

    #[test]
    fn test_signverify_good() {
        let secret = OpaqueSecret::new();
        let message = b"123123121";
        let signature = secret.sign(message).unwrap();
        assert!(secret.verify(message, &signature).unwrap());
    }

    #[test]
    fn test_signverify_bad() {
        let secret = OpaqueSecret::new();
        let message = b"123123121";
        let signature = b"aojdasjod";
        assert_eq!(
            secret.verify(message, &*signature),
            Err(SecretError::InvalidSignatureFormat)
        );
    }
}

fn secret_new() -> Box<OpaqueSecret> {
    Box::new(OpaqueSecret::new())
}

fn secret_from_str(s: &str) -> Result<Box<OpaqueSecret>, SecretError> {
    Ok(Box::new(OpaqueSecret::from_str(s)?))
}

impl OpaqueSecret {
    fn c_derive(&self, ty: c_char) -> Result<Box<Self>, SecretError> {
        Ok(Box::new(self.derive(ty as u8 as char)?))
    }

    fn c_clone(&self) -> Box<Self> {
        Box::new(self.clone())
    }

    fn c_get_private(&self) -> Result<Vec<u8>, SecretError> {
        Ok(self.get_private_key()?.to_bytes().to_vec())
    }

    fn c_get_symmetric(&self) -> Result<Vec<u8>, SecretError> {
        Ok(self.get_symmetric_key()?.to_vec())
    }

    fn c_get_public(&self) -> Result<Vec<u8>, SecretError> {
        Ok(self.get_public_key()?.to_bytes().to_vec())
    }
}

#[cxx::bridge]
mod ffx {
    extern "Rust" {
        type OpaqueSecret;
        fn to_string(self: &OpaqueSecret) -> String;
        fn secret_new() -> Box<OpaqueSecret>;
        fn secret_from_str(s: &str) -> Result<Box<OpaqueSecret>>;
        fn c_derive(self: &OpaqueSecret, ty: c_char) -> Result<Box<OpaqueSecret>>;
        fn c_clone(&self) -> Box<OpaqueSecret>;
        fn sign(&self, message: &[u8]) -> Result<Vec<u8>>;
        fn verify(&self, message: &[u8], signature: &[u8]) -> Result<bool>;
        fn c_get_private(&self) -> Result<Vec<u8>>;
        fn c_get_symmetric(&self) -> Result<Vec<u8>>;
        fn c_get_public(&self) -> Result<Vec<u8>>;
    }
}
