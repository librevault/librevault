#pragma once
#include <QJsonDocument>
#include <QObject>
#include <QVariant>

namespace librevault {

class AbstractConfig : public QObject {
  Q_OBJECT

 signals:
  void globalChanged(QString key, QVariant value);
  void folderAdded(QVariantMap fconfig);
  void folderRemoved(QByteArray folderid);

 public:
  /* Global configuration */
  virtual QVariant getGlobal(QString name) = 0;
  virtual void setGlobal(QString name, QVariant value) = 0;
  virtual void removeGlobal(QString name) = 0;

  /* Folder configuration */
  virtual void addFolder(QVariantMap fconfig) = 0;
  virtual void removeFolder(QByteArray folderid) = 0;

  virtual QVariantMap getFolder(QByteArray folderid) = 0;
  virtual QVariant getFolderValue(QByteArray folderid, const QString& name) { return getFolder(folderid).value(name); }
  virtual QList<QByteArray> listFolders() = 0;

  /* Export/Import */
  virtual QJsonDocument exportUserGlobals() = 0;
  virtual QJsonDocument exportGlobals() = 0;
  virtual void importGlobals(QJsonDocument globals_conf) = 0;

  virtual QJsonDocument exportUserFolders() = 0;
  virtual QJsonDocument exportFolders() = 0;
  virtual void importFolders(QJsonDocument folders_conf) = 0;
};

}  // namespace librevault
