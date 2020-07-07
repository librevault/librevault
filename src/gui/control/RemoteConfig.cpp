#include "RemoteConfig.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "Daemon.h"
#include "Secret.h"

RemoteConfig::RemoteConfig(Daemon* daemon) : daemon_(daemon) {
  connect(daemon_, &Daemon::connected, this, &RemoteConfig::renew);
  connect(daemon_, &Daemon::eventReceived, this, &RemoteConfig::handleEvent);

  connect(this, &AbstractConfig::globalChanged, this, &RemoteConfig::changed);
  connect(this, &AbstractConfig::folderAdded, this, &RemoteConfig::changed);
  connect(this, &AbstractConfig::folderRemoved, this, &RemoteConfig::changed);
}

QVariant RemoteConfig::getGlobal(QString name) { return cached_globals_[name]; }

void RemoteConfig::setGlobal(QString name, QVariant value) {
  if ((getGlobal(name) != value) && daemon_->isConnected()) {
    QJsonObject value_setter;
    value_setter["key"] = name;
    value_setter["value"] = QJsonValue::fromVariant(value);

    QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/globals/%1").arg(name));
    daemon_->nam()->put(request, QJsonDocument(value_setter).toJson(QJsonDocument::Compact));
  }
}

void RemoteConfig::removeGlobal(QString name) {
  if (cached_globals_.contains(name) && daemon_->isConnected()) {
    QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/globals/%1").arg(name));
    daemon_->nam()->deleteResource(request);
  }
}

void RemoteConfig::addFolder(QVariantMap fconfig) {
  if (daemon_->isConnected()) {
    librevault::Secret secret(fconfig["secret"].toString());
    QByteArray folderid = secret.get_Hash();

    QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/folders/%1").arg(QString(folderid.toHex())));
    daemon_->nam()->put(request, QJsonDocument::fromVariant(fconfig).toJson());
  }
}

void RemoteConfig::removeFolder(QByteArray folderid) {
  if (daemon_->isConnected()) {
    QNetworkRequest request(daemon_->daemonUrl().toString().append("/v1/folders/%1").arg(QString(folderid.toHex())));
    QNetworkReply* reply = daemon_->nam()->deleteResource(request);

    QEventLoop sync_loop;
    connect(reply, &QNetworkReply::finished, &sync_loop, &QEventLoop::quit);
    sync_loop.exec();

    if (reply->error()) {
      // handle somehow!
    }
  }
}

QVariantMap RemoteConfig::getFolder(QByteArray folderid) { return cached_folders_.value(folderid); }

QList<QByteArray> RemoteConfig::listFolders() { return cached_folders_.keys(); }

QJsonDocument RemoteConfig::exportUserGlobals() {
  qFatal("User values are not supported on client side");
  return {};
}

QJsonDocument RemoteConfig::exportGlobals() { return QJsonDocument::fromVariant(cached_globals_); }

void RemoteConfig::importGlobals(QJsonDocument globals_conf) { Q_ASSERT(!"Not implemented yet"); }

QJsonDocument RemoteConfig::exportUserFolders() {
  qFatal("User values are not supported on client side");
  return {};
}

QJsonDocument RemoteConfig::exportFolders() {
  QJsonArray folders;
  for (const QVariantMap& folder : cached_folders_.values()) {
    folders.append(QJsonValue::fromVariant(folder));
  }
  return QJsonDocument(folders);
}

void RemoteConfig::importFolders(QJsonDocument folders_conf) { Q_ASSERT(!"Not implemented yet"); }

void RemoteConfig::renew() {
  {
    QUrl request_url = daemon_->daemonUrl();
    request_url.setPath("/v1/globals");
    QNetworkRequest request(request_url);
    QNetworkReply* reply = daemon_->nam()->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, request_url] {
      if (reply->error() == QNetworkReply::NoError) {
        handleGlobals(QJsonDocument::fromJson(reply->readAll()));
        qDebug() << "Fetched:" << request_url;
      }
    });
  }

  {
    QUrl request_url = daemon_->daemonUrl();
    request_url.setPath("/v1/folders");
    QNetworkRequest request(request_url);
    QNetworkReply* reply = daemon_->nam()->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, request_url] {
      if (reply->error() == QNetworkReply::NoError) {
        handleFolders(QJsonDocument::fromJson(reply->readAll()));
        qDebug() << "Fetched:" << request_url;
      }
    });
  }
}

void RemoteConfig::handleGlobals(QJsonDocument globals) {
  QVariantMap globals_new = globals.object().toVariantMap();

  // Prepare notifications
  QStringList all_keys = globals_new.keys() + cached_globals_.keys();
  all_keys.removeDuplicates();

  QMutableListIterator<QString> it(all_keys);
  while (it.hasNext()) {
    QString key = it.next();
    if (globals_new.value(key) == cached_globals_.value(key)) it.remove();
  }

  // Set
  cached_globals_ = globals_new;

  // Notify
  for (const QString& key : all_keys) {
    globalChanged(key, cached_globals_.value(key));
  }
}

void RemoteConfig::handleFolders(QJsonDocument folders) {
  for (const QJsonValue& value : folders.array()) {
    QJsonObject folder_object = value.toObject();
    librevault::Secret secret(folder_object["secret"].toString());
    cached_folders_[secret.get_Hash()] = folder_object.toVariantMap();
  }
}

void RemoteConfig::handleEvent(QString name, QJsonObject event) {
  if (name == "EVENT_GLOBAL_CONFIG_CHANGED") {
    QString key = event["key"].toString();
    QJsonValue value = event["value"];
    if (cached_globals_.value(key) != value) {
      cached_globals_[key] = value;
      emit globalChanged(key, value);
    }
  } else if (name == "EVENT_FOLDER_ADDED") {
    QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
    cached_folders_[folderid] = event["folder_params"].toObject().toVariantMap();
    emit folderAdded(cached_folders_[folderid]);
  } else if (name == "EVENT_FOLDER_REMOVED") {
    QByteArray folderid = QByteArray::fromHex(event["folderid"].toString().toLatin1());
    cached_folders_.remove(folderid);
    emit folderRemoved(folderid);
  }
}
