#pragma once
#include <QDialog>

#include "ui_SettingsWindow.h"
#include "ui_SettingsWindow_bottom.h"

class SettingsWindowPrivate;

class SettingsWindow : public QDialog {
  Q_OBJECT

  friend class SettingsWindowPrivate;

 public:
  explicit SettingsWindow(QWidget* parent = 0);
  ~SettingsWindow();

  /**
   * Adds a new preferences pane to Settings window.
   * An icon will be taken from pane.windowIcon
   * The title will be taken from pane.windowTitle
   * This SettingsWindow object will take ownership of pane
   * @param pane
   * @return
   */
  void addPane(QWidget* pane);

 public slots:
  void retranslateUi();

 protected:
  Ui::SettingsWindow ui_window_;
  Ui::SettingsWindow_bottom ui_bottom_;

 protected:
  SettingsWindowPrivate* d;
};
