#include "AesCbcData.h"

#include "Meta.h"
#include "crypto/AES_CBC.h"

namespace librevault {

QByteArray AesCbcData::ct() const { return ct_; }
QByteArray AesCbcData::iv() const { return iv_; }

void AesCbcData::setEncrypted(const std::string& ct, const std::string& iv) {
  ct_ = QByteArray::fromStdString(ct);
  iv_ = QByteArray::fromStdString(iv);
}

QByteArray AesCbcData::get_plain(const Secret& secret) const {
  try {
    return ct_ | crypto::De<crypto::AES_CBC>(secret.get_Encryption_Key(), iv_);
  } catch (const CryptoPP::Exception& e) {
    throw Meta::parse_error("Parse error: Decryption failed");
  }
}

void AesCbcData::set_plain(const QByteArray& pt, const Secret& secret) {
  iv_ = crypto::AES_CBC::randomIv();
  ct_ = pt | crypto::AES_CBC(secret.get_Encryption_Key(), iv_);
}

}  // namespace librevault
