use std::convert::TryFrom;
use std::ffi::CStr;
use std::fs::{read, File};
use std::io::Write;
use std::os::raw::c_char;

use hex;
use log::trace;
use openssl::asn1::Asn1Time;
use openssl::ec::{EcGroup, EcKey};
use openssl::error::ErrorStack;
use openssl::hash::MessageDigest;
use openssl::nid::Nid;
use openssl::pkey::PKey;
use openssl::x509::{X509Builder, X509Name, X509};

#[no_mangle]
pub extern "C" fn nodekey_write_new(key_path: *const c_char) {
    let key_path = unsafe { CStr::from_ptr(key_path) };
    let curve = EcGroup::from_curve_name(Nid::X9_62_PRIME256V1).unwrap();
    let keypair = EcKey::generate(curve.as_ref()).unwrap();
    let pem = keypair.private_key_to_pem().unwrap();

    let mut key_file = File::create(key_path.to_str().unwrap()).unwrap();
    key_file.write_all(pem.as_slice()).unwrap();
}

fn build_certificate(private_key_pem: &[u8]) -> Result<X509, ErrorStack> {
    let keypair = EcKey::private_key_from_pem(private_key_pem)?;
    let pkey = &PKey::try_from(keypair)?;

    let mut cert_builder = X509Builder::new()?;
    cert_builder.set_pubkey(&pkey)?;

    cert_builder.set_version(0)?;
    cert_builder.set_not_before(Asn1Time::days_from_now(0)?.as_ref())?;
    cert_builder.set_not_after(Asn1Time::days_from_now(365 * 10)?.as_ref())?;

    let mut name = X509Name::builder()?;
    name.append_entry_by_nid(Nid::COMMONNAME, "Librevault")?;
    let name = name.build();

    cert_builder.set_subject_name(&name)?;
    cert_builder.set_issuer_name(&name)?;
    cert_builder.sign(&pkey, MessageDigest::sha256())?;

    Ok(cert_builder.build())
}

#[no_mangle]
pub extern "C" fn nodekey_write_new_cert(key_path: *const c_char, cert_path: *const c_char) {
    let key_path = unsafe { CStr::from_ptr(key_path) };
    let cert_path = unsafe { CStr::from_ptr(cert_path) };

    let key = read(key_path.to_str().unwrap()).unwrap();
    let certificate = build_certificate(key.as_slice()).unwrap();

    let pem = certificate.to_pem().unwrap();
    trace!(
        "PEM certificate: {}",
        String::from_utf8_lossy(pem.as_slice())
    );
    trace!(
        "PEM digest: {}",
        hex::encode(certificate.digest(MessageDigest::sha256()).unwrap())
    );

    let mut key_file = File::create(cert_path.to_str().unwrap()).unwrap();
    key_file.write_all(pem.as_slice()).unwrap();
}
