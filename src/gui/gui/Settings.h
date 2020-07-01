#pragma once
#include "gui/settings/SettingsWindow.h"
#include "startup/StartupInterface.h"
#include "ui_Settings_General.h"
#include "ui_Settings_Network.h"

class Daemon;
class Updater;
class Translator;

class Settings : public SettingsWindow {
  Q_OBJECT

 public:
  explicit Settings(Daemon* daemon, Updater* updater, Translator* translator, QWidget* parent = 0);
  ~Settings();

 signals:
  void localeChanged(QString code);

 public slots:
  void retranslateUi();

 protected:
  Ui::Settings_General ui_pane_general_;
  Ui::Settings_Network ui_pane_network_;
  QWidget* pane_general_;
  QWidget* pane_network_;

  void init_ui();
  void reset_ui_states();
  void process_ui_states();

  Daemon* daemon_;
  Updater* updater_;
  Translator* translator_;
  StartupInterface* startup_interface_;

 private:
  void showEvent(QShowEvent* e) override;

 private slots:
  void okayPressed();
  void cancelPressed();
};
