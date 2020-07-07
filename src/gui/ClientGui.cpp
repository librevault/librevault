#include "ClientGui.h"

#include <QCommandLineParser>
#include <QFileOpenEvent>
#include <QLibraryInfo>
#include <QTimer>

#include "control/Daemon.h"
#include "control/RemoteConfig.h"
#include "control/RemoteState.h"
#include "gui/MainWindow.h"
#include "model/FolderModel.h"
#include "translator/Translator.h"

ClientGui::ClientGui(int &argc, char **argv, int appflags) : QtSingleApplication("com.librevault.desktop", argc, argv) {
  setAttribute(Qt::AA_UseHighDpiPixmaps);
  setQuitOnLastWindowClosed(false);

  translator_ = new Translator(this);

  // Parsing arguments
  QCommandLineParser parser;
  QCommandLineOption attach_option(QStringList() << "a"
                                                 << "attach",
                                   tr("Attach to running daemon instead of creating a new one"), "url");
  QCommandLineOption multi_option("multi", tr("Allow multiple instances of GUI"));
  parser.addOption(attach_option);
  parser.addOption(multi_option);
  parser.addPositionalArgument("url", tr("The \"lvlt:\" URL to open."));
  parser.process(*this);

  if (!parser.positionalArguments().empty()) {
    QRegularExpression link_regex("lvlt:.*");
    if (auto link_index = parser.positionalArguments().indexOf(link_regex); link_index != -1)
      pending_link_ = parser.positionalArguments().at(link_index);
  }

  allow_multiple = parser.isSet(multi_option);

  // Creating components
  daemon_ = new Daemon(parser.value(attach_option), this);
  folder_model_ = new FolderModel(daemon_, this);
  updater_ = new Updater(this);
  main_window_ = new MainWindow(daemon_, folder_model_, updater_, translator_);

  // Connecting signals & slots
  //	connect(main_window_, &MainWindow::newConfigIssued, control_client_, &ControlClient::sendConfigJson);
  //  	connect(main_window_, &MainWindow::folderAdded, control_client_, &ControlClient::sendAddFolderJson);
  //  	connect(main_window_, &MainWindow::folderRemoved, control_client_, &ControlClient::sendRemoveFolderJson);
  connect(this, &QtSingleApplication::messageReceived, this, &ClientGui::singleMessageReceived);

  connect(daemon_, &Daemon::connected, main_window_, &MainWindow::handle_connected);
  connect(daemon_, &Daemon::disconnected, main_window_, &MainWindow::handle_disconnected);

  connect(daemon_->config(), &RemoteConfig::changed, folder_model_, &FolderModel::refresh);
  connect(daemon_->state(), &RemoteState::changed, folder_model_, &FolderModel::refresh);

  // Initialization complete!
  QTimer::singleShot(0, this, &ClientGui::started);
}

ClientGui::~ClientGui() { delete main_window_; }

bool ClientGui::event(QEvent *event) {
  if (event->type() == QEvent::FileOpen) {
    auto open_event = dynamic_cast<QFileOpenEvent *>(event);
    pending_link_ = open_event->url().toString();
  }
  return QApplication::event(event);
}

void ClientGui::started() {
  if (allow_multiple || !isRunning()) {
    daemon_->start();
    if (!pending_link_.isEmpty()) {
      main_window_->handleLink(pending_link_);
      pending_link_.clear();
    }
  } else {
    // Send message to an active instance
    QJsonObject message;
    if (pending_link_.isEmpty()) {
      message["action"] = "show";
    } else {
      message["action"] = "link";
      message["link"] = pending_link_;
    }
    sendMessage(QJsonDocument(message).toJson());
    quit();
  }
}

void ClientGui::singleMessageReceived(const QString &message_str) {
  qDebug() << "SingleApplication: " << message_str;
  auto message = QJsonDocument::fromJson(message_str.toUtf8());
  if (message["action"] == "show") {
    main_window_->showWindow();
  } else if (message["action"] == "link") {
    main_window_->handleLink(message["link"].toString());
  }
}
