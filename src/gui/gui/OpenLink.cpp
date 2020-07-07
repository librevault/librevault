#include "OpenLink.h"

#include "MainWindow.h"

OpenLink::OpenLink(QWidget* parent) : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  ui.setupUi(this);
}

void OpenLink::accept() {
  if (qobject_cast<MainWindow*>(parent())->handleLink(ui.line_url->text()) && isVisible()) QDialog::accept();
}
