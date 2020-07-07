#pragma once
#include <QJsonValue>

#include "AbstractConfig.h"
#include "GenericRemoteDictionary.h"

class Daemon;

class RemoteConfig : public librevault::AbstractConfig {
  Q_OBJECT

 signals:
  void changed();

 public:
  explicit RemoteConfig(Daemon* daemon);
  ~RemoteConfig() override = default;

 public slots:
  /* Global configuration */
  QVariant getGlobal(QString name) override;
  void setGlobal(QString name, QVariant value) override;
  void removeGlobal(QString name) override;

  /* Folder configuration */
  void addFolder(QVariantMap fconfig) override;
  void removeFolder(QByteArray folderid) override;

  QVariantMap getFolder(QByteArray folderid) override;
  QList<QByteArray> listFolders() override;

  /* Export/Import */
  QJsonDocument exportUserGlobals() override;
  QJsonDocument exportGlobals() override;
  void importGlobals(QJsonDocument globals_conf) override;

  QJsonDocument exportUserFolders() override;
  QJsonDocument exportFolders() override;
  void importFolders(QJsonDocument folders_conf) override;

 private:
  QVariantMap cached_globals_;
  QMap<QByteArray, QVariantMap> cached_folders_;

  Daemon* daemon_;

  void renew();
  void handleGlobals(QJsonDocument globals);
  void handleFolders(QJsonDocument folders);
  void handleEvent(QString name, QJsonObject event);
};
