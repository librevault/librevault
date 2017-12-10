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
#include "Meta.h"
#include <memory>

namespace librevault {

class SignedMeta {
public:
	struct signature_error : Meta::error {
		signature_error() : Meta::error("Meta signature mismatch") {}
	};

	SignedMeta() {}
	SignedMeta(Meta meta, const Secret& secret);
	SignedMeta(QByteArray raw_meta, QByteArray signature);

	operator bool() const {return meta_ && !raw_meta_.isEmpty() && !signature_.isEmpty();}

	bool isValid(const Secret& secret) const;

	// Getters
	const Meta& meta() const {return *meta_;}
	QByteArray raw_meta() const {return raw_meta_;}
	QByteArray signature() const {return signature_;}
private:
	std::shared_ptr<Meta> meta_;

	QByteArray raw_meta_;
	QByteArray signature_;
};

} /* namespace librevault */
