#pragma once
#include <QJsonObject>
#include <QMainWindow>
#include <QMenu>
#include <QSystemTrayIcon>
#include <memory>

#include "gui/AddFolder.h"
#include "gui/OpenLink.h"
#include "gui/Settings.h"
#include "ui_MainWindow.h"

class FolderModel;
class FolderProperties;
class StatusBar;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(Daemon* daemon, FolderModel* folder_model, Updater* updater, Translator* translator);
  ~MainWindow() override = default;

 public slots:
  void showWindow();
  void retranslateUi();
  void openWebsite();
  bool handleLink(const QString& link);

  void handle_disconnected(const QString& message);
  void handle_connected();

 protected slots:
  void tray_icon_activated(QSystemTrayIcon::ActivationReason reason);
  void showAddFolderDialog(QJsonObject folder_config = QJsonObject());
  void showOpenLinkDialog();
  void showFolderContextMenu(const QPoint& point);
  void handleRemoveFolder();
  void handleOpenFolderProperties();
  void handleOpenDestinationFolder();

 protected:
  /* UI elements */
  Ui::MainWindow ui;
  StatusBar* status_bar_;

  /* Models */
  FolderModel* folder_model_;

  /* Dialogs */
  Settings* settings_;
  QMap<QByteArray, FolderProperties*> folder_properties_windows_;

 protected:
  /* Event handlers (overrides) */
  void changeEvent(QEvent* e) override;
  // void closeEvent(QCloseEvent* e) override;

  /* Actions */
  QAction* show_main_window_action;
  QAction* open_website_action;
  QAction* show_settings_window_action;
  QAction* exit_action;
  QAction* new_folder_action;
  QAction* open_link_action;
  QAction* delete_folder_action;
  QAction* folder_properties_action;
  QAction* folder_destination_action;

  void init_actions();
  void init_toolbar();

  /* Tray icon */
  QSystemTrayIcon tray_icon;
  QMenu tray_context_menu;

  void init_tray();

  /* Daemon */
  Daemon* daemon_;
};
