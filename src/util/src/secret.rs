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
pub enum Secret {
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

impl Secret {
    pub fn new() -> Self {
        let mut rng = OsRng::default();
        let keypair: Keypair = Keypair::generate(&mut rng);

        Secret::Signer {
            symmetric_key: Sha3_256::digest(&keypair.secret.to_bytes()).to_vec(),
            public_key: keypair.public,
            private_key: keypair.secret,
        }
    }

    fn get_private_key(&self) -> Result<&SecretKey, SecretError> {
        match self {
            Secret::Signer { private_key, .. } => Ok(&private_key),
            _ => Err(SecretError::DeriveError),
        }
    }

    pub fn get_symmetric_key(&self) -> Result<&[u8], SecretError> {
        match self {
            Secret::Signer { symmetric_key, .. }
            | Secret::Decryptor { symmetric_key, .. } => Ok(&symmetric_key),
            _ => Err(SecretError::DeriveError),
        }
    }

    fn get_public_key(&self) -> Result<&PublicKey, SecretError> {
        match self {
            Secret::Signer { public_key, .. }
            | Secret::Decryptor { public_key, .. }
            | Secret::Verifier { public_key, .. } => Ok(&public_key),
        }
    }

    fn derive(&self, ty: char) -> Result<Self, SecretError> {
        match ty {
            'A' => Ok(Secret::Signer {
                private_key: SecretKey::from_bytes(&self.get_private_key()?.to_bytes()).unwrap(),
                symmetric_key: self.get_symmetric_key()?.to_vec(),
                public_key: self.get_public_key()?.clone(),
            }),
            'B' => Ok(Secret::Decryptor {
                symmetric_key: self.get_symmetric_key()?.to_vec(),
                public_key: self.get_public_key()?.clone(),
            }),
            'C' => Ok(Secret::Verifier {
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
    
    pub fn get_id(&self) -> Vec<u8> {
        Sha3_256::digest(self.get_public_key().unwrap().as_bytes()).to_vec()
    }
}

impl FromStr for Secret {
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
                Secret::Signer {
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
                Secret::Decryptor {
                    symmetric_key,
                    public_key,
                }
            }),
            'C' => Ok({
                Secret::Verifier {
                    public_key: PublicKey::from_bytes(&decrypted_payload)
                        .ok()
                        .ok_or(SecretError::CryptoError)?,
                }
            }),
            _ => Err(SecretError::InvalidType),
        }
    }
}

impl ToString for Secret {
    fn to_string(&self) -> String {
        let (ty, payload) = match self {
            Secret::Signer { private_key, .. } => ('A', private_key.to_bytes().to_vec()),
            Secret::Decryptor {
                public_key,
                symmetric_key,
            } => ('B', {
                [&public_key.to_bytes()[..], symmetric_key.as_slice()].concat()
            }),
            Secret::Verifier { public_key } => ('C', public_key.to_bytes().to_vec()),
        };
        let encoded_payload = bs58::encode(payload).into_string();
        let luhn = LUHNGENERATOR.generate(&encoded_payload).unwrap();
        format!("{}{}{}{}", ty, CURRENT_VERSION, encoded_payload, luhn)
    }
}

impl Clone for Secret {
    fn clone(&self) -> Self {
        match self {
            Secret::Signer {
                private_key,
                public_key,
                symmetric_key,
            } => Secret::Signer {
                private_key: SecretKey::from_bytes(&private_key.to_bytes()[..]).unwrap(),
                public_key: *public_key,
                symmetric_key: symmetric_key.clone(),
            },
            Secret::Decryptor {
                public_key,
                symmetric_key,
            } => Secret::Decryptor {
                public_key: *public_key,
                symmetric_key: symmetric_key.clone(),
            },
            Secret::Verifier { public_key } => Secret::Verifier {
                public_key: *public_key,
            },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip() {
        let secret = Secret::new();
        let serialized = secret.to_string();
        let parsed = Secret::from_str(serialized.as_str()).unwrap();
        let serialized_once_more = parsed.to_string();
        assert_eq!(serialized, serialized_once_more);
    }

    #[test]
    fn test_clone() {
        let secret1 = Secret::new();
        let secret2 = secret1.clone();
        assert_eq!(secret1.to_string(), secret2.to_string());
    }

    #[test]
    fn test_signverify_good() {
        let secret = Secret::new();
        let message = b"123123121";
        let signature = secret.sign(message).unwrap();
        assert!(secret.verify(message, &signature).unwrap());
    }

    #[test]
    fn test_signverify_bad() {
        let secret = Secret::new();
        let message = b"123123121";
        let signature = b"aojdasjod";
        assert_eq!(
            secret.verify(message, &*signature),
            Err(SecretError::InvalidSignatureFormat)
        );
    }
}

fn secret_new() -> Box<Secret> {
    Box::new(Secret::new())
}

fn secret_from_str(s: &str) -> Result<Box<Secret>, SecretError> {
    Ok(Box::new(Secret::from_str(s)?))
}

impl Secret {
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
        #[cxx_name = "OpaqueSecret"]
        type Secret;
        fn to_string(self: &Secret) -> String;
        fn secret_new() -> Box<Secret>;
        fn secret_from_str(s: &str) -> Result<Box<Secret>>;
        fn c_derive(self: &Secret, ty: c_char) -> Result<Box<Secret>>;
        fn c_clone(&self) -> Box<Secret>;
        fn sign(&self, message: &[u8]) -> Result<Vec<u8>>;
        fn verify(&self, message: &[u8], signature: &[u8]) -> Result<bool>;
        fn c_get_private(&self) -> Result<Vec<u8>>;
        fn c_get_symmetric(&self) -> Result<Vec<u8>>;
        fn c_get_public(&self) -> Result<Vec<u8>>;
    }
}
