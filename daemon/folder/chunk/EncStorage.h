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
#pragma once
#include "blob.h"
#include "util/log.h"
#include <QFile>
#include <QReadWriteLock>
#include <memory>

namespace librevault {

class FolderParams;
class EncStorage : public QObject {
	Q_OBJECT
	LOG_SCOPE("EncStorage");
public:
	EncStorage(const FolderParams& params, QObject* parent);

	bool have_chunk(QByteArray ct_hash) const noexcept;
	QByteArray get_chunk(QByteArray ct_hash) const;
	void put_chunk(QByteArray ct_hash, QFile* chunk_f);
	void remove_chunk(QByteArray ct_hash);

private:
	const FolderParams& params_;
	mutable QReadWriteLock storage_mtx_;

	QString make_chunk_ct_name(QByteArray ct_hash) const noexcept;
	QString make_chunk_ct_path(QByteArray ct_hash) const noexcept;
};

} /* namespace librevault */
