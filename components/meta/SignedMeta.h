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
#include "MetaInfo.h"
#include <QMetaType>

namespace librevault {

class SignedMeta {
public:
	struct signature_error : MetaInfo::error {
		signature_error() : MetaInfo::error("Meta signature mismatch") {}
	};

	SignedMeta() = default;
	SignedMeta(const MetaInfo& meta, const Secret& secret);
	SignedMeta(const QByteArray& raw_meta, const QByteArray& signature);

	operator bool() const {return !meta_info_.isEmpty() && !raw_meta_info_.isEmpty() && !signature_.isEmpty();}

	bool isValid(const Secret& secret) const;

	// Getters
	const MetaInfo& metaInfo() const {return meta_info_;}
	const QByteArray& rawMetaInfo() const {return raw_meta_info_;}
	const QByteArray& signature() const {return signature_;}
private:
	MetaInfo meta_info_;

	QByteArray raw_meta_info_;
	QByteArray signature_;
};

} /* namespace librevault */

Q_DECLARE_METATYPE(librevault::SignedMeta)
