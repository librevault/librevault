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

#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <QDir>

#include "control/Paths.h"
#include "util/log.h"

Q_LOGGING_CATEGORY(log_nodekey, "nodekey")

namespace librevault {

NodeKey::NodeKey(QObject* parent) : QObject(parent) {
  SCOPELOG(log_nodekey);
  cert_file_.setFileName(QDir::fromNativeSeparators(Paths::get()->cert_path));
  private_key_file_.setFileName(QDir::fromNativeSeparators(Paths::get()->key_path));

  write_key();
  gen_certificate();

  private_key_file_.open(QIODevice::ReadOnly);
  cert_file_.open(QIODevice::ReadOnly);

  private_key_ = QSslKey(&private_key_file_, QSsl::Ec);
  certificate_ = QSslCertificate(&cert_file_);

  private_key_file_.close();
  cert_file_.close();

  qCInfo(log_nodekey) << "PeerID:" << digest().toHex();
}

NodeKey::~NodeKey() { SCOPELOG(log_nodekey); }

QByteArray NodeKey::digest() const { return certificate().digest(digestAlgorithm()); }

void NodeKey::write_key() {
  /* Generate key */
  CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP> private_key;

  CryptoPP::AutoSeededRandomPool rng;
  private_key.Initialize(rng, CryptoPP::ASN1::secp256r1());

  //	std::string s;
  //	CryptoPP::StringSink ss(s);
  //	private_key.DEREncode(ss);
  //
  //	private_key_ = QSslKey(QByteArray::fromRawData(s.data(), s.size()), QSsl::Ec, QSsl::Der);
  //	private_key_file_.open(QIODevice::WriteOnly | QIODevice::Truncate);
  //	private_key_file_.write(private_key_.toPem());
  //	private_key_file_.close();

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
    throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "PEM_DEREncode: failed to retrieve curve OID");

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

  // Converting to PEM and writing to file
  private_key_file_.open(QIODevice::WriteOnly | QIODevice::Truncate);
  QSslKey der_key(QByteArray::fromStdString(der_buffer), QSsl::Ec, QSsl::Der);
  private_key_file_.write(der_key.toPem());
  private_key_file_.close();
}

void NodeKey::gen_certificate() {
  std::unique_ptr<X509, decltype(&X509_free)> x509(X509_new(), &X509_free);
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> openssl_pkey(EVP_PKEY_new(), &EVP_PKEY_free);

  // Get private key
  private_key_file_.open(QIODevice::ReadOnly);
  QByteArray private_key_pem = private_key_file_.readAll();
  private_key_file_.close();

  {  // Read private key from buffer
    std::unique_ptr<BIO, decltype(&BIO_free)> private_key_bio(
        BIO_new_mem_buf(private_key_pem.data(), private_key_pem.size()), &BIO_free);
    EVP_PKEY* evp_pkey = openssl_pkey.get();
    PEM_read_bio_PrivateKey(private_key_bio.get(), &evp_pkey, 0, 0);
  }

  /* Set the serial number. */
  ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1);

  /* This certificate is valid from now until exactly one year from now. */
  X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
  X509_gmtime_adj(X509_get_notAfter(x509.get()), 31536000L);

  /* Set the public key for our certificate. */
  X509_set_pubkey(x509.get(), openssl_pkey.get());

  /* We want to copy the subject name to the issuer name. */
  X509_NAME* name = X509_get_subject_name(x509.get());

  /* Set the country code and common name. */
  // X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *) "CA", -1, -1, 0);
  // X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *) "MyCompany", -1, -1, 0);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"Librevault", -1, -1,
                             0);  // Use some sort of user-agent

  /* Now set the issuer name. */
  X509_set_issuer_name(x509.get(), name);

  /* Actually sign the certificate with our key. */
  if (!X509_sign(x509.get(), openssl_pkey.get(), EVP_sha256())) {
    X509_free(x509.get());
    throw std::runtime_error("Error signing certificate.");
  }

  /* Write certificate to DER buffer */
  QByteArray der_buffer(i2d_X509(x509.get(), nullptr), '\0');
  auto data_ptr = (uchar*)der_buffer.data();
  i2d_X509(x509.get(), &data_ptr);

  /* Write the certificate to disk. */
  cert_file_.open(QIODevice::WriteOnly | QIODevice::Truncate);
  QSslCertificate der_cert(der_buffer, QSsl::Der);
  cert_file_.write(der_cert.toPem());
  cert_file_.close();
}

}  // namespace librevault
