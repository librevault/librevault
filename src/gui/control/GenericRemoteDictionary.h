#pragma once
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QObject>

class Daemon;

class GenericRemoteDictionary : public QObject {
  Q_OBJECT

 public:
  explicit GenericRemoteDictionary(Daemon* daemon, QString globals_request, QString folders_request,
                                   QString global_event, QString folder_event);
  ~GenericRemoteDictionary() {}

  QVariant getGlobalValue(QString key);
  QJsonValue getFolderValue(QByteArray folderid, QString key);
  QList<QByteArray> folderList();

 signals:
  void changed();

 protected:
  Daemon* daemon_;
  QJsonObject global_cache_;
  QMap<QByteArray, QJsonObject> folder_cache_;

  QString convertOutValue(QJsonValue value);

 private:
  QString globals_request_;
  QString folders_request_;

  QString global_event_;
  QString folder_event_;

 private slots:
  void renew();
  void handleEvent(QString name, QJsonObject event);
};
