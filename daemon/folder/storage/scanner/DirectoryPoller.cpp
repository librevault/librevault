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
#include "IndexerQueue.h"
#include "folder/storage/Storage.h"
#include "folder/storage/Index.h"
#include "control/FolderParams.h"
#include "folder/IgnoreList.h"
#include <PathNormalizer.h>
#include <QDirIterator>

namespace librevault {

DirectoryPoller::DirectoryPoller(const FolderParams& params, IgnoreList* ignore_list, Storage* parent) :
	QObject(parent),
	params_(params),
	meta_storage_(parent),
	ignore_list_(ignore_list) {

	polling_timer_ = new QTimer(this);
	polling_timer_->setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(params_.full_rescan_interval));
	polling_timer_->setTimerType(Qt::VeryCoarseTimer);

	connect(polling_timer_, &QTimer::timeout, this, &DirectoryPoller::addPathsToQueue);
}

DirectoryPoller::~DirectoryPoller() = default;

void DirectoryPoller::setEnabled(bool enabled) {
	if(enabled) {
		QTimer::singleShot(0, this, &DirectoryPoller::addPathsToQueue);
		polling_timer_->start();
	}
	else
		polling_timer_->stop();
}

QList<QString> DirectoryPoller::getReindexList() {
	QSet<QString> file_list;

	// Files present in the file system
	qDebug() << params_.path;
	QDirIterator dir_it(
		params_.path,
		QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
		params_.preserve_symlinks ? (QDirIterator::Subdirectories) : (QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
	);
	while(dir_it.hasNext()) {
		QString abspath = dir_it.next();
		QByteArray normpath = PathNormalizer::normalizePath(abspath, params_.path);

		if(!ignore_list_->isIgnored(normpath)) file_list.insert(abspath);
	}

	// Prevent incomplete (not assembled, partially-downloaded, whatever) from periodical scans.
	// They can still be indexed by monitor, though.
	for(auto& smeta : meta_storage_->index()->getIncompleteMeta()) {
		QString denormpath = PathNormalizer::absolutizePath(smeta.metaInfo().path().plaintext(params_.secret.encryptionKey()), params_.path);
		file_list.remove(denormpath);
	}

	// Files present in index (files added from here will be marked as DELETED)
	for(auto& smeta : meta_storage_->index()->getExistingMeta()) {
		QByteArray normpath = smeta.metaInfo().path().plaintext(params_.secret.encryptionKey());
		QString denormpath = PathNormalizer::absolutizePath(normpath, params_.path);

		if(!ignore_list_->isIgnored(normpath)) file_list.insert(denormpath);
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
