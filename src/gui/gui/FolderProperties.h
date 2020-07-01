#pragma once
#include <QDialog>
#include <QJsonObject>
#include <memory>

#include "Secret.h"
#include "ui_FolderProperties.h"

class Daemon;
class FolderModel;

class FolderProperties : public QDialog {
  Q_OBJECT

 public:
  FolderProperties(QByteArray folderid, Daemon* daemon, FolderModel* folder_model, QWidget* parent = 0);
  ~FolderProperties();

 protected:
  Ui::FolderProperties ui;
  void init_secrets();

 public slots:
  void refresh();

 private:
  Daemon* daemon_;
  FolderModel* folder_model_;
  QByteArray folderid_;
};
