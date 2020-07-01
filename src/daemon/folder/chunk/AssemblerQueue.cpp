#include "AssemblerQueue.h"

#include <QLoggingCategory>

#include "AssemblerWorker.h"
#include "folder/meta/MetaStorage.h"

Q_DECLARE_LOGGING_CATEGORY(log_assembler)

namespace librevault {

AssemblerQueue::AssemblerQueue(const FolderParams& params, MetaStorage* meta_storage, ChunkStorage* chunk_storage,
                               PathNormalizer* path_normalizer, Archive* archive, QObject* parent)
    : QObject(parent),
      params_(params),
      meta_storage_(meta_storage),
      chunk_storage_(chunk_storage),
      path_normalizer_(path_normalizer),
      archive_(archive) {
  threadpool_ = new QThreadPool(this);

  assemble_timer_ = new QTimer(this);
  assemble_timer_->setInterval(30 * 1000);
  connect(assemble_timer_, &QTimer::timeout, this, &AssemblerQueue::periodicAssembleOperation);
  assemble_timer_->start();
}

AssemblerQueue::~AssemblerQueue() {
  qCDebug(log_assembler) << "Stopping assembler queue";
  emit aboutToStop();
  threadpool_->waitForDone();
  qCDebug(log_assembler) << "Assembler queue stopped";
}

void AssemblerQueue::addAssemble(SignedMeta smeta) {
  AssemblerWorker* worker =
      new AssemblerWorker(smeta, params_, meta_storage_, chunk_storage_, path_normalizer_, archive_);
  worker->setAutoDelete(true);
  threadpool_->start(worker);
}

void AssemblerQueue::periodicAssembleOperation() {
  qCDebug(log_assembler) << "Performing periodic assemble";

  for (const auto& smeta : meta_storage_->getIncompleteMeta()) addAssemble(smeta);
}

}  // namespace librevault
