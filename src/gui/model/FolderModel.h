#pragma once
#include <QAbstractTableModel>
#include <QJsonObject>

class Daemon;
class PeerModel;

class FolderModel : public QAbstractListModel {
  Q_OBJECT

 public:
  FolderModel(Daemon* daemon, QObject* parent);
  ~FolderModel() override = default;

  static const int HashRole = Qt::UserRole;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

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
