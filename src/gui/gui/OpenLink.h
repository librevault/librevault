#pragma once
#include <QDialog>

#include "ui_OpenLink.h"

class OpenLink : public QDialog {
  Q_OBJECT

 public:
  explicit OpenLink(QWidget* parent);

 public slots:
  void accept() override;

 protected:
  Ui::OpenLink ui;
};
