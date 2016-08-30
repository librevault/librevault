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

#include <QtCore>

inline QString human_size(uintmax_t size) {
	float num = size;

	if(num < 1024.0)
		return QCoreApplication::translate("Human Size", "%n bytes", 0, size);
	num /= 1024.0;

	if(num < 1024.0)
		return QCoreApplication::translate("Human Size", "%1 KB").arg(num, 0, 'f', 0);
	num /= 1024.0;

	if(num < 1024.0)
		return QCoreApplication::translate("Human Size", "%1 MB").arg(num, 0, 'f', 2);
	num /= 1024.0;

	if(num < 1024.0)
		return QCoreApplication::translate("Human Size", "%1 GB").arg(num, 0, 'f', 2);
	num /= 1024.0;

	return QCoreApplication::translate("Human Size", "%1 TB").arg(num, 0, 'f', 2);
}

inline QString human_bandwidth(float bandwidth) {
	if(bandwidth < 1024.0)
		return QCoreApplication::translate("Human Bandwidth", "%1 B/s").arg(bandwidth, 0, 'f', 0);
	bandwidth /= 1024.0;

	if(bandwidth < 1024.0)
		return QCoreApplication::translate("Human Bandwidth", "%1 KB/s").arg(bandwidth, 0, 'f', 1);
	bandwidth /= 1024.0;

	if(bandwidth < 1024.0)
		return QCoreApplication::translate("Human Bandwidth", "%1 MB/s").arg(bandwidth, 0, 'f', 1);
	bandwidth /= 1024.0;

	if(bandwidth < 1024.0)
		return QCoreApplication::translate("Human Bandwidth", "%1 GB/s").arg(bandwidth, 0, 'f', 1);
	bandwidth /= 1024.0;

	return QCoreApplication::translate("Human Bandwidth", "%1 TB/s").arg(bandwidth, 0, 'f', 1);
}
