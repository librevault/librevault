use std::fmt::{Debug, Display, Formatter};
use std::str::FromStr;

use ed25519_dalek_fiat::{Keypair, PublicKey, SecretKey, Signer, Verifier, PUBLIC_KEY_LENGTH};
use hex::ToHex;
use lazy_static::lazy_static;
use luhn::Luhn;
use rand::rngs::OsRng;
use serde::{Deserialize, Serialize};
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
        write!(f, "{self}")
    }
}

#[derive(Serialize, Deserialize)]
#[serde(try_from = "String", into = "String")]
pub enum Secret {
    Signer {
        private_key: SecretKey,
        symmetric_key: Vec<u8>,
        public_key: PublicKey,
    },
    Decrypter {
        symmetric_key: Vec<u8>,
        public_key: PublicKey,
    },
    Verifier {
        public_key: PublicKey,
    },
}

const CURRENT_VERSION: char = '2';

impl Default for Secret {
    fn default() -> Self {
        Self::random()
    }
}

impl Secret {
    pub fn random() -> Self {
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
            Secret::Signer { private_key, .. } => Ok(private_key),
            _ => Err(SecretError::DeriveError),
        }
    }

    pub fn get_symmetric_key(&self) -> Result<&[u8], SecretError> {
        match self {
            Secret::Signer { symmetric_key, .. } | Secret::Decrypter { symmetric_key, .. } => {
                Ok(&symmetric_key)
            }
            _ => Err(SecretError::DeriveError),
        }
    }

    fn get_public_key(&self) -> Result<&PublicKey, SecretError> {
        match self {
            Secret::Signer { public_key, .. }
            | Secret::Decrypter { public_key, .. }
            | Secret::Verifier { public_key, .. } => Ok(public_key),
        }
    }

    pub fn derive(&self, ty: char) -> Result<Self, SecretError> {
        match ty {
            'A' => Ok(Secret::Signer {
                private_key: SecretKey::from_bytes(&self.get_private_key()?.to_bytes()).unwrap(),
                symmetric_key: self.get_symmetric_key()?.to_vec(),
                public_key: *self.get_public_key()?,
            }),
            'B' => Ok(Secret::Decrypter {
                symmetric_key: self.get_symmetric_key()?.to_vec(),
                public_key: *self.get_public_key()?,
            }),
            'C' => Ok(Secret::Verifier {
                public_key: *self.get_public_key()?,
            }),
            _ => Err(SecretError::DeriveError),
        }
    }

    pub fn sign(&self, message: &[u8]) -> Result<Vec<u8>, SecretError> {
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

    pub fn verify(&self, message: &[u8], signature: &[u8]) -> Result<bool, SecretError> {
        let signature = ed25519::Signature::from_bytes(signature)
            .ok()
            .ok_or(SecretError::InvalidSignatureFormat)?;
        Ok(self.get_public_key()?.verify(message, &signature).is_ok())
    }

    pub fn get_id(&self) -> Vec<u8> {
        Sha3_256::digest(self.get_public_key().unwrap().as_bytes()).to_vec()
    }

    pub fn get_id_hex(&self) -> String {
        self.get_id().encode_hex()
    }
}

impl Display for Secret {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", String::from(self))
    }
}

impl Debug for Secret {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Secret(\"{self}\")")
    }
}

impl FromStr for Secret {
    type Err = SecretError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let secret_type = s.chars().next().ok_or(SecretError::ParseFailed)?;
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
                Secret::Decrypter {
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

impl TryFrom<String> for Secret {
    type Error = SecretError;

    fn try_from(value: String) -> Result<Self, Self::Error> {
        Secret::from_str(value.as_str())
    }
}

impl From<&Secret> for String {
    fn from(secret: &Secret) -> Self {
        let (ty, payload) = match secret {
            Secret::Signer { private_key, .. } => ('A', private_key.to_bytes().to_vec()),
            Secret::Decrypter {
                public_key,
                symmetric_key,
            } => ('B', {
                [&public_key.to_bytes()[..], symmetric_key.as_slice()].concat()
            }),
            Secret::Verifier { public_key } => ('C', public_key.to_bytes().to_vec()),
        };
        let encoded_payload = bs58::encode(payload).into_string();
        let luhn = LUHNGENERATOR.generate(&encoded_payload).unwrap();
        format!("{ty}{CURRENT_VERSION}{encoded_payload}{luhn}")
    }
}

impl From<Secret> for String {
    fn from(secret: Secret) -> Self {
        String::from(&secret)
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
            Secret::Decrypter {
                public_key,
                symmetric_key,
            } => Secret::Decrypter {
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
