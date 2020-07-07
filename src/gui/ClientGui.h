#pragma once
#include <QApplication>
#include <QTranslator>
#include <QtSingleApplication>

#include "updater/Updater.h"

class MainWindow;
class Settings;
class TrayIcon;
class FolderModel;
class Daemon;
class RemoteConfig;
class Translator;

class ClientGui : public QtSingleApplication {
  Q_OBJECT

 public:
  ClientGui(int& argc, char** argv, int appflags = ApplicationFlags);
  ~ClientGui();

  bool event(QEvent* event);

 private:
  Translator* translator_;
  Daemon* daemon_;
  FolderModel* folder_model_;

  Updater* updater_;

  // GUI
  MainWindow* main_window_;

  QString pending_link_;

  bool allow_multiple;

 private slots:
  void started();
  void singleMessageReceived(const QString& message_str);
};
