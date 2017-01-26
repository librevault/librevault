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
#include "DirectoryPoller.h"
#include "Index.h"
#include "Indexer.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include "folder/PathNormalizer.h"
#include "util/log.h"
#include <QSet>
#include <boost/filesystem.hpp>

namespace librevault {

DirectoryPoller::DirectoryPoller(const FolderParams& params, Index* index, IgnoreList* ignore_list, PathNormalizer* path_normalizer, QObject* parent) :
	QObject(parent),
	params_(params),
	index_(index),
	ignore_list_(ignore_list),
	path_normalizer_(path_normalizer) {

	polling_timer_ = new QTimer(this);
	polling_timer_->setInterval(params_.full_rescan_interval.count());
	polling_timer_->setTimerType(Qt::VeryCoarseTimer);

	connect(polling_timer_, &QTimer::timeout, this, &DirectoryPoller::addPathsToQueue);
}

DirectoryPoller::~DirectoryPoller() {}

void DirectoryPoller::setEnabled(bool enabled) {
	if(enabled)
		polling_timer_->start();
	else
		polling_timer_->stop();
}

QList<QString> DirectoryPoller::getReindexList() {
	QSet<QString> file_list;

	// Files present in the file system
	for(auto dir_entry_it = boost::filesystem::recursive_directory_iterator(params_.path); dir_entry_it != boost::filesystem::recursive_directory_iterator(); dir_entry_it++){
		QString abspath = QString::fromStdWString(dir_entry_it->path().wstring());
		QByteArray normpath = path_normalizer_->normalizePath(abspath);

		if(!ignore_list_->is_ignored(normpath.toStdString())) file_list.insert(abspath);
	}

	// Prevent incomplete (not assembled, partially-downloaded, whatever) from periodical scans.
	// They can still be indexed by monitor, though.
	for(auto& smeta : index_->get_incomplete_meta()) {
		QString denormpath = path_normalizer_->denormalizePath(QByteArray::fromStdString(smeta.meta().path(params_.secret)));
		file_list.remove(denormpath);
	}

	// Files present in index (files added from here will be marked as DELETED)
	for(auto& smeta : index_->get_existing_meta()) {
		QByteArray normpath = QByteArray::fromStdString(smeta.meta().path(params_.secret));
		QString denormpath = path_normalizer_->denormalizePath(normpath);

		if(!ignore_list_->is_ignored(normpath.toStdString())) file_list.insert(denormpath);
	}

	return file_list.toList();
}

void DirectoryPoller::addPathsToQueue() {
	LOGD("Performing full directory rescan");

	foreach(QString denormpath, getReindexList()) {
		emit newPath(denormpath);
	}
}

} /* namespace librevault */
