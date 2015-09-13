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
#include "../../Session.h"
#include "../../contrib/crypto/Hex.h"
#include "../../contrib/crypto/Base64.h"

namespace librevault {

NodeKey::NodeKey(Session& session) :
		session_(session), log_(session_.log()),
		openssl_pkey_(EVP_PKEY_new()), x509_(X509_new()) {
	gen_private_key();
	write_key();
	gen_certificate();
}

NodeKey::~NodeKey() {
	EVP_PKEY_free(openssl_pkey_);
	X509_free(x509_);
}

CryptoPP::DL_PrivateKey_EC<CryptoPP::ECP>& NodeKey::gen_private_key() {
	CryptoPP::AutoSeededRandomPool rng;
	private_key_.Initialize(rng, CryptoPP::ASN1::secp256r1());
	CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> public_key;

	private_key_.MakePublicKey(public_key);
	public_key.AccessGroupParameters().SetPointCompression(true);

	public_key_.resize(33);
	public_key.GetGroupParameters().EncodeElement(true, public_key.GetPublicElement(), public_key_.data());

	log_->debug() << "Public key: " << (std::string)crypto::Hex().to(public_key_);

	return private_key_;
}

void NodeKey::write_key(){
	fs::ofstream ofs(session_.key_path(), std::ios_base::binary);

	ofs << "-----BEGIN EC PRIVATE KEY-----" << std::endl;
	auto& group_params = private_key_.GetGroupParameters();

	bool old = group_params.GetEncodeAsOID();
	const_cast<CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>&>(group_params).SetEncodeAsOID(true);

    CryptoPP::DL_PublicKey_EC<CryptoPP::ECP> pkey;
    private_key_.MakePublicKey(pkey);

    std::string s;
    CryptoPP::StringSink ss(s);
    CryptoPP::DERSequenceEncoder seq(ss);
    CryptoPP::DEREncodeUnsigned<CryptoPP::word32>(seq, 1);

    // Private key
    const CryptoPP::Integer& x = private_key_.GetPrivateExponent();
    x.DEREncodeAsOctetString(seq, group_params.GetSubgroupOrder().ByteCount());

    // Named curve
    CryptoPP::OID oid;
    if(!private_key_.GetVoidValue(CryptoPP::Name::GroupOID(), typeid(oid), &oid))
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

    for (unsigned i = 0; i < s.length(); i += 64) {
    	ofs << s.substr(i, 64) << std::endl;
    }

	const_cast<CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>&>(group_params).SetEncodeAsOID(old);
	ofs << "-----END EC PRIVATE KEY-----" << std::endl;
}

void NodeKey::gen_certificate() {
	FILE* f = fopen(session_.key_path().string().c_str(), "r");

	PEM_read_PrivateKey(f, &openssl_pkey_, 0, 0);
	fclose(f);

	/* Set the serial number. */
	ASN1_INTEGER_set(X509_get_serialNumber(x509_), 1);

	/* This certificate is valid from now until exactly one year from now. */
	X509_gmtime_adj(X509_get_notBefore(x509_), 0);
	X509_gmtime_adj(X509_get_notAfter(x509_), 31536000L);

	/* Set the public key for our certificate. */
	X509_set_pubkey(x509_, openssl_pkey_);

	/* We want to copy the subject name to the issuer name. */
	X509_NAME * name = X509_get_subject_name(x509_);

	/* Set the country code and common name. */
	//X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *) "CA", -1, -1, 0);
	//X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *) "MyCompany", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *) "Librevault", -1, -1, 0);	// Use some sort of user-agent

	/* Now set the issuer name. */
	X509_set_issuer_name(x509_, name);

	/* Actually sign the certificate with our key. */
	if (!X509_sign(x509_, openssl_pkey_, EVP_sha256())) {
		X509_free(x509_);
		throw std::runtime_error("Error signing certificate.");
	}

	/* Open the PEM file for writing the certificate to disk. */
	FILE * x509_file = fopen(session_.cert_path().string().c_str(), "wb");
	if (!x509_file) {
		throw std::runtime_error("Unable to open \"cert.pem\" for writing.");
	}

	/* Write the certificate to disk. */
	PEM_write_X509(x509_file, x509_);
	fclose(x509_file);
}

} /* namespace librevault */
