#pragma once
#include <QAbstractTableModel>
#include <QJsonObject>

class Daemon;
class PeerModel;

class FolderModel : public QAbstractListModel {
  Q_OBJECT

 public:
  FolderModel(Daemon* daemon);
  ~FolderModel();

  static const int HashRole = Qt::UserRole;

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  PeerModel* getPeerModel(const QByteArray& hash) { return peer_models_[hash]; }

 public slots:
  void refresh();

 private:
  Daemon* daemon_;

  QList<QByteArray> folders_all_;
  QMap<QByteArray, PeerModel*> peer_models_;

  enum class Column {
    NAME,
    STATUS,
    PEERS,
    SIZE,

    COLUMN_COUNT
  };
};
