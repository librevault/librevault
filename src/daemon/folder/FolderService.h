#pragma once
#include <QMap>
#include <QObject>

#include "util/blob.h"
#include "util/log.h"

Q_DECLARE_LOGGING_CATEGORY(log_folderservice)

namespace librevault {

/* Folder info */
class FolderGroup;
class FolderParams;
class StateCollector;

class FolderService : public QObject {
  Q_OBJECT
  LOG_SCOPE("FolderService");

 public:
  struct invalid_group : std::runtime_error {
    invalid_group() : std::runtime_error("Folder group not found") {}
  };

  explicit FolderService(StateCollector* state_collector, QObject* parent);
  virtual ~FolderService();

  void run();
  void stop();

  /* FolderGroup nanagenent */
  void initFolder(const FolderParams& params);
  void deinitFolder(const QByteArray& folderid);

  FolderGroup* getGroup(const QByteArray& folderid);

 signals:
  void folderAdded(FolderGroup* fgroup);
  void folderRemoved(FolderGroup* fgroup);

 private:
  StateCollector* state_collector_;

  QMap<QByteArray, FolderGroup*> groups_;
};

}  // namespace librevault
