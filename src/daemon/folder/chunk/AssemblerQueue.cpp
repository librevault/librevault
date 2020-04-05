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
#include "AssemblerQueue.h"
#include "AssemblerWorker.h"
#include "folder/meta/MetaStorage.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(log_assembler)

namespace librevault {

AssemblerQueue::AssemblerQueue(const FolderParams& params,
                             MetaStorage* meta_storage,
                             ChunkStorage* chunk_storage,
                             PathNormalizer* path_normalizer,
                             Archive* archive, QObject* parent) :
	QObject(parent),
	params_(params),
	meta_storage_(meta_storage),
	chunk_storage_(chunk_storage),
	path_normalizer_(path_normalizer),
	archive_(archive) {

	threadpool_ = new QThreadPool(this);

	assemble_timer_ = new QTimer(this);
	assemble_timer_->setInterval(30*1000);
	connect(assemble_timer_, &QTimer::timeout, this, &AssemblerQueue::periodic_assemble_operation);
	assemble_timer_->start();
}

AssemblerQueue::~AssemblerQueue() {
	qCDebug(log_assembler) << "Stopping assembler queue";
	emit aboutToStop();
	threadpool_->waitForDone();
	qCDebug(log_assembler) << "Assembler queue stopped";
}

void AssemblerQueue::addAssemble(SignedMeta smeta) {
	AssemblerWorker* worker = new AssemblerWorker(smeta, params_, meta_storage_, chunk_storage_, path_normalizer_, archive_);
	worker->setAutoDelete(true);
	threadpool_->start(worker);
}

void AssemblerQueue::periodic_assemble_operation() {
	qCDebug(log_assembler) << "Performing periodic assemble";

	for(auto smeta : meta_storage_->getIncompleteMeta())
		addAssemble(smeta);
}

} /* namespace librevault */
