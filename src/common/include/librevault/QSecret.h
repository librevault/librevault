/* Copyright (C) 2015 Alexander Shishenko <alex@shishenko.com>
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
 */
#pragma once
#include "librevault/util/blob.h"
#include <librevault/Secret.h>
#include <QByteArray>
#include <QString>
#include <memory>

namespace librevault {

class QSecret {
public:
	QSecret() {secret_ = std::make_shared<Secret>();}
	QSecret(Secret::Type type, QByteArray payload) {secret_ = std::make_shared<Secret>(type, conv_bytearray(payload));}
	QSecret(QString string_secret) {secret_ = std::make_shared<Secret>(string_secret.toStdString());}

	QSecret(Secret secret) {secret_ = std::make_shared<Secret>(secret);}

	QString string() const {return QString::fromStdString(secret_->string());}
	operator QString() const {return string();}

	Secret::Type get_type() const {return secret_->get_type();}
	char get_param() const {return secret_->get_param();}
	char get_check_char() const {return secret_->get_check_char();}

	// Secret derivers
	QSecret derive(Secret::Type key_type) const {return QSecret(secret_->derive(key_type));}

	// Payload getters
	QByteArray get_Private_Key() const {return conv_bytearray(secret_->get_Private_Key());}
	QByteArray get_Public_Key() const {return conv_bytearray(secret_->get_Public_Key());}
	QByteArray get_Encryption_Key() const {return conv_bytearray(secret_->get_Encryption_Key());}

	QByteArray get_Hash() const {return conv_bytearray(secret_->get_Hash());}

	bool operator== (QSecret key) const {return *secret_ == *(key.secret_);}
	bool operator< (QSecret key) const {return *secret_ < *(key.secret_);}

private:
	std::shared_ptr<Secret> secret_;
};

} /* namespace librevault */
