#pragma once
#include <QAbstractTableModel>
#include <QJsonObject>

class Daemon;
class FolderModel;

class PeerModel : public QAbstractListModel {
  Q_OBJECT

 public:
  PeerModel(QByteArray folderid, Daemon* daemon, FolderModel* parent);
  ~PeerModel();

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

 public slots:
  void refresh();

 private:
  Daemon* daemon_;
  QByteArray folderid_;

  enum class Column {
    CLIENT_NAME,
    ENDPOINT,
    USER_AGENT,
    DOWN_SPEED,
    UP_SPEED,
    DOWN_BYTES,
    UP_BYTES,

    COLUMN_COUNT
  };
};
