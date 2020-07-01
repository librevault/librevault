#include "AddFolder.h"

#include <QFileDialog>
#include <QJsonObject>
#include <QShowEvent>

#include "Secret.h"
#include "control/Daemon.h"
#include "control/RemoteConfig.h"

AddFolder::AddFolder(QString secret, Daemon* daemon, QWidget* parent) : QDialog(parent), daemon_(daemon) {
  setAttribute(Qt::WA_DeleteOnClose);
  ui.setupUi(this);

  ui.line_Secret->setText(secret);

  connect(ui.button_CreateSecret, &QPushButton::clicked, this, &AddFolder::generateSecret);
  connect(ui.button_Browse, &QPushButton::clicked, this, &AddFolder::browseFolder);
}

AddFolder::~AddFolder() {}

void AddFolder::accept() {
  auto secret = ui.line_Secret->text();
  auto path = ui.line_Folder->text();

  QVariantMap folder_config;
  folder_config["secret"] = secret;
  folder_config["path"] = path;
  daemon_->config()->addFolder(folder_config);

  QDialog::accept();
}

void AddFolder::showEvent(QShowEvent* e) {
  adjustSize();
  QDialog::showEvent(e);
}

void AddFolder::generateSecret() {
  librevault::Secret s;
  ui.line_Secret->setText(s.string());
}

void AddFolder::browseFolder() {
  QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the directory to sync"), QString(),
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  ui.line_Folder->setText(dir);
}
