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

class Client : public QtSingleApplication {
  Q_OBJECT

 public:
  Client(int& argc, char** argv, int appflags = ApplicationFlags);
  ~Client();

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
  void singleMessageReceived(const QString& message);
};
