/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "NodeKey.h"
#include "control/Paths.h"
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <QDir>

namespace librevault {

Q_LOGGING_CATEGORY(log_nodekey, "nodekey");

NodeKey::NodeKey(QObject* parent) : QObject(parent) {
  private_key_ = generateKey();
  certificate_ = createCertificate(private_key_);

  // Writing private key
  QFile private_key_file(Paths::get()->key_path);
  private_key_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  private_key_file.write(generateKey().toPem());
  private_key_file.close();

  // Writing certificate
  QFile cert_file(Paths::get()->cert_path);
  cert_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  cert_file.write(createCertificate(private_key_).toPem());
  cert_file.close();

  qCDebug(log_nodekey) << "PeerID:" << digest().toHex();
}

QByteArray NodeKey::digest() const { return certificate().digest(digestAlgorithm()); }

QSslConfiguration NodeKey::getSslConfiguration() const {
  QSslConfiguration ssl_config;
  ssl_config.setPeerVerifyMode(QSslSocket::QueryPeer);
  ssl_config.setPrivateKey(privateKey());
  ssl_config.setLocalCertificate(certificate());
  ssl_config.setProtocol(QSsl::TlsV1_2OrLater);
  return ssl_config;
}

QSslKey NodeKey::generateKey() {
  /* Generate key */
  CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;

  CryptoPP::AutoSeededRandomPool rng;
  private_key.Initialize(rng, CryptoPP::ASN1::secp256r1());

  /* Write to DER buffer */
  auto& group_params = private_key.GetGroupParameters();

  bool old = group_params.GetEncodeAsOID();
  const_cast<CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>&>(group_params).SetEncodeAsOID(true);

  CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> pkey;
  private_key.MakePublicKey(pkey);

  std::string der_buffer;
  CryptoPP::StringSink ss(der_buffer);
  CryptoPP::DERSequenceEncoder seq(ss);
  CryptoPP::DEREncodeUnsigned<CryptoPP::word32>(seq, 1);

  // Private key
  const CryptoPP::Integer& x = private_key.GetPrivateExponent();
  x.DEREncodeAsOctetString(seq, group_params.GetSubgroupOrder().ByteCount());

  // Named curve
  CryptoPP::OID oid;
  if (!private_key.GetVoidValue(CryptoPP::Name::GroupOID(), typeid(oid), &oid))
    throw CryptoPP::Exception(
        CryptoPP::Exception::OTHER_ERROR, "PEM_DEREncode: failed to retrieve curve OID");

  // Encoder for OID
  CryptoPP::DERGeneralEncoder cs1(seq, CryptoPP::CONTEXT_SPECIFIC | CryptoPP::CONSTRUCTED | 0);
  oid.DEREncode(cs1);
  cs1.MessageEnd();

  // Encoder for public key (outer CONTEXT_SPECIFIC)
  CryptoPP::DERGeneralEncoder cs2(seq, CryptoPP::CONTEXT_SPECIFIC | CryptoPP::CONSTRUCTED | 1);

  // Encoder for public key (inner BIT_STRING)
  CryptoPP::DERGeneralEncoder cs3(cs2, CryptoPP::BIT_STRING);
  cs3.Put(0x00);  // Unused bits
  group_params.GetCurve().EncodePoint(cs3, pkey.GetPublicElement(), false);

  // Done encoding
  cs3.MessageEnd();
  cs2.MessageEnd();

  // Sequence end
  seq.MessageEnd();

  const_cast<CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>&>(group_params).SetEncodeAsOID(old);

  // Converting to PEM
  return QSslKey(QByteArray::fromStdString(der_buffer), QSsl::Ec, QSsl::Der);
}

QSslCertificate NodeKey::createCertificate(const QSslKey& key) {
  std::unique_ptr<X509, decltype(&X509_free)> x509(X509_new(), &X509_free);
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> openssl_pkey(EVP_PKEY_new(), &EVP_PKEY_free);

  // Read private key from buffer
  {
    QByteArray private_key_pem = key.toPem();
    std::unique_ptr<BIO, decltype(&BIO_free)> private_key_bio(
        BIO_new_mem_buf(private_key_pem.data(), private_key_pem.size()), &BIO_free);
    EVP_PKEY* evp_pkey = openssl_pkey.get();
    PEM_read_bio_PrivateKey(private_key_bio.get(), &evp_pkey, 0, 0);
  }

  /* Set the serial number. */
  ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1);

  /* This certificate is valid from now until exactly one year from now. */
  X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
  X509_gmtime_adj(X509_get_notAfter(x509.get()), 60l * 60l * 24l * 365l);

  /* Set the public key for our certificate. */
  X509_set_pubkey(x509.get(), openssl_pkey.get());

  {
    /* We want to copy the subject name to the issuer name. */
    X509_NAME* name = X509_get_subject_name(x509.get());

    /* Set the country code and common name. */
    // X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (uchar*) "CA", -1, -1, 0);
    // X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (uchar*) "MyCompany", -1, -1, 0);
    X509_NAME_add_entry_by_txt(
        name, "CN", MBSTRING_ASC, (uchar*)"Librevault", -1, -1, 0);  // Use some sort of user-agent

    /* Now set the issuer name. */
    X509_set_issuer_name(x509.get(), name);
  }

  /* Actually sign the certificate with our key. */
  if (!X509_sign(x509.get(), openssl_pkey.get(), EVP_sha256())) {
    X509_free(x509.get());
    throw std::runtime_error("Error signing certificate.");
  }

  /* Write certificate to DER buffer */
  QByteArray der_buffer(i2d_X509(x509.get(), nullptr), '\0');
  uchar* data_ptr = (uchar*)der_buffer.data();
  i2d_X509(x509.get(), &data_ptr);

  /* Write the certificate to disk. */
  return QSslCertificate(der_buffer, QSsl::Der);
}

} /* namespace librevault */
