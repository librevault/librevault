#include "Client.h"

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

Client::Client(int &argc, char **argv, int appflags) : QtSingleApplication("com.librevault.desktop", argc, argv) {
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
    auto link_index = parser.positionalArguments().indexOf(link_regex);
    if (link_index != -1) pending_link_ = parser.positionalArguments().at(link_index);
  }

  allow_multiple = parser.isSet(multi_option);

  // Creating components
  daemon_ = new Daemon(parser.value(attach_option), this);
  folder_model_ = new FolderModel(daemon_);
  updater_ = new Updater(this);
  main_window_ = new MainWindow(daemon_, folder_model_, updater_, translator_);

  // Connecting signals & slots
  //	connect(main_window_, &MainWindow::newConfigIssued, control_client_, &ControlClient::sendConfigJson);
  //	connect(main_window_, &MainWindow::folderAdded, control_client_, &ControlClient::sendAddFolderJson);
  //	connect(main_window_, &MainWindow::folderRemoved, control_client_, &ControlClient::sendRemoveFolderJson);
  connect(this, &QtSingleApplication::messageReceived, this, &Client::singleMessageReceived);

  connect(daemon_, &Daemon::connected, main_window_, &MainWindow::handle_connected);
  connect(daemon_, &Daemon::disconnected, main_window_, &MainWindow::handle_disconnected);

  connect(daemon_->config(), &RemoteConfig::changed, folder_model_, &FolderModel::refresh);
  connect(daemon_->state(), &RemoteState::changed, folder_model_, &FolderModel::refresh);

  // Initialization complete!
  QTimer::singleShot(0, [this] { started(); });
}

Client::~Client() { delete main_window_; }

bool Client::event(QEvent *event) {
  if (event->type() == QEvent::FileOpen) {
    QFileOpenEvent *open_event = static_cast<QFileOpenEvent *>(event);
    pending_link_ = open_event->url().toString();
  }
  return QApplication::event(event);
}

void Client::started() {
  if (allow_multiple || !isRunning()) {
    daemon_->start();
    if (!pending_link_.isEmpty()) {
      main_window_->handleLink(pending_link_);
      pending_link_.clear();
    }
  } else {
    if (pending_link_.isEmpty())
      sendMessage("show");
    else
      sendMessage(QString("link ").append(pending_link_));
    quit();
  }
}

void Client::singleMessageReceived(const QString &message) {
  qDebug() << "SingleApplication: " << message;
  if (message == "show") {
    main_window_->showWindow();
  } else if (message.startsWith("link")) {
    QString link = message.right(message.size() - 5);
    main_window_->handleLink(link);
  }
}
