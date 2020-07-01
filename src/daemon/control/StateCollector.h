#pragma once
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QObject>

namespace librevault {

class StateCollector : public QObject {
  Q_OBJECT
 public:
  StateCollector(QObject* parent);
  ~StateCollector();

  void global_state_set(QString key, QJsonValue value);
  void folder_state_set(QByteArray folderid, QString key, QJsonValue value);
  void folder_state_purge(QByteArray folderid);

  QJsonObject global_state();
  QJsonArray folder_state();
  QJsonObject folder_state(const QByteArray& folderid);

 signals:
  void globalStateChanged(QString key, QJsonValue value);
  void folderStateChanged(QByteArray folderid, QString key, QJsonValue value);

 private:
  QJsonObject global_state_buffer;
  QMap<QByteArray, QJsonObject> folder_state_buffers;
};

}  // namespace librevault
