#include "SettingsWindow.h"
#ifndef Q_OS_MAC
#include "SettingsWindowPrivate.all.h"
#else
#include "SettingsWindowPrivate.mac.h"
#endif

SettingsWindow::SettingsWindow(QWidget* parent) : QDialog(parent) {
  ui_window_.setupUi(this);
  d = new SettingsWindowPrivate(this);

  //	connect(ui_bottom_.dialog_box, &QDialogButtonBox::accepted, this, &Settings::okayPressed);
  //	connect(ui_bottom_.dialog_box, &QDialogButtonBox::rejected, this, &Settings::cancelPressed);
}

SettingsWindow::~SettingsWindow() { delete d; }

void SettingsWindow::addPane(QWidget* pane) {
  pane->setParent(this);
  d->add_pane(pane);
}

void SettingsWindow::retranslateUi() {}
