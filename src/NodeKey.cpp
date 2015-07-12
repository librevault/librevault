/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "NodeKey.h"
#include "../contrib/crypto/Base64.h"

namespace librevault {

NodeKey::NodeKey(fs::path key, fs::path cert) :
		key(key), cert(cert), openssl_pkey(EVP_PKEY_new()) {
	gen_private_key();
	write_key();
	gen_certificate();
}

NodeKey::~NodeKey() {
	EVP_PKEY_free(openssl_pkey);
}

CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP>& NodeKey::gen_private_key() {
	CryptoPP::AutoSeededRandomPool rng;
	private_key.Initialize(rng, CryptoPP::ASN1::secp256r1());

	return private_key;
}

void NodeKey::write_key(){
	fs::ofstream ofs(key, std::ios_base::binary);

	ofs.write("-----BEGIN EC PRIVATE KEY-----\n", 31);
	auto& group_params = private_key.GetGroupParameters();

	bool old = group_params.GetEncodeAsOID();
	const_cast<CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>&>(group_params).SetEncodeAsOID(true);

    CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> pkey;
    private_key.MakePublicKey(pkey);

    std::string s;
    CryptoPP::StringSink ss(s);
    CryptoPP::DERSequenceEncoder seq(ss);
    CryptoPP::DEREncodeUnsigned<CryptoPP::word32>(seq, 1);

    // Private key
    const CryptoPP::Integer& x = private_key.GetPrivateExponent();
    x.DEREncodeAsOctetString(seq, group_params.GetSubgroupOrder().ByteCount());

    // Named curve
    CryptoPP::OID oid;
    if(!private_key.GetVoidValue(CryptoPP::Name::GroupOID(), typeid(oid), &oid))
        throw CryptoPP::Exception(CryptoPP::Exception::OTHER_ERROR, "PEM_DEREncode: failed to retrieve curve OID");

    // Encoder for OID
    CryptoPP::DERGeneralEncoder cs1(seq, CryptoPP::CONTEXT_SPECIFIC | CryptoPP::CONSTRUCTED | 0);
    oid.DEREncode(cs1);
    cs1.MessageEnd();

    // Encoder for public key (outer CONTEXT_SPECIFIC)
    CryptoPP::DERGeneralEncoder cs2(seq, CryptoPP::CONTEXT_SPECIFIC | CryptoPP::CONSTRUCTED | 1);

    // Encoder for public key (inner BIT_STRING)
    CryptoPP::DERGeneralEncoder cs3(cs2, CryptoPP::BIT_STRING);
    cs3.Put(0x00);        // Unused bits
    group_params.GetCurve().EncodePoint(cs3, pkey.GetPublicElement(), false);

    // Done encoding
    cs3.MessageEnd();
    cs2.MessageEnd();

    // Sequence end
    seq.MessageEnd();

    s = (std::string)crypto::Base64().to(s);

    std::string result;

    for (unsigned i = 0; i < s.length(); i += 64) {
    	result += s.substr(i, 64);
    	result += '\n';
    }

    ofs.write(result.data(), result.size());

	const_cast<CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>&>(group_params).SetEncodeAsOID(old);
	ofs.write("-----END EC PRIVATE KEY-----\n", 29);
}

void NodeKey::gen_certificate() {
	FILE* f = fopen(key.c_str(), "r");
	PEM_read_PrivateKey(f, &openssl_pkey, 0, 0);
	fclose(f);

	/* Allocate memory for the X509 structure. */
	X509* x509 = X509_new();
	if (!x509) {throw std::runtime_error("Unable to create X509 structure.");}

	/* Set the serial number. */
	ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

	/* This certificate is valid from now until exactly one year from now. */
	X509_gmtime_adj(X509_get_notBefore(x509), 0);
	X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);

	/* Set the public key for our certificate. */
	X509_set_pubkey(x509, openssl_pkey);

	/* We want to copy the subject name to the issuer name. */
	X509_NAME * name = X509_get_subject_name(x509);

	/* Set the country code and common name. */
	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *) "CA", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *) "MyCompany", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *) "localhost", -1, -1, 0);

	/* Now set the issuer name. */
	X509_set_issuer_name(x509, name);

	/* Actually sign the certificate with our key. */
	if (!X509_sign(x509, openssl_pkey, EVP_sha256())) {
		X509_free(x509);
		throw std::runtime_error("Error signing certificate.");
	}

	/* Open the PEM file for writing the certificate to disk. */
	FILE * x509_file = fopen(cert.c_str(), "wb");
	if (!x509_file) {
		X509_free(x509);
		throw std::runtime_error("Unable to open \"cert.pem\" for writing.");
	}

	/* Write the certificate to disk. */
	PEM_write_X509(x509_file, x509);
	fclose(x509_file);
}

} /* namespace librevault */
