#pragma once
#include <QDialog>

#include "ui_AddFolder.h"

class Daemon;
class AddFolder : public QDialog {
  Q_OBJECT

 public:
  AddFolder(QString secret, Daemon* daemon, QWidget* parent);
  ~AddFolder() override;

 protected:
  Ui::AddFolder ui;
  Daemon* daemon_;

  // Overrides
  void showEvent(QShowEvent* e) override;

 private slots:
  void accept() override;

  void generateSecret();
  void browseFolder();
  // void validateSecret();
};
