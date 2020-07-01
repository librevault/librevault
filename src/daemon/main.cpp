#include <docopt/docopt.h>
#include <openssl/ssl.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <util/conv_fspath.h>

#include <QDebug>
#include <boost/filesystem/path.hpp>
#ifdef Q_OS_UNIX
#include <csignal>
#endif

#include "Client.h"
#include "Secret.h"
#include "Version.h"
#include "control/Config.h"
#include "control/Paths.h"

using namespace librevault;  // This is allowed only because this is main.cpp
// file and it is extremely unlikely that this file
// will be included in any other file.

// clang-format off

///////////////////////////////////////////////////////////////////////80 chars/
static const char* USAGE =
    R"(Librevault synchronization daemon.

Librevault is an open source peer-to-peer file synchronization solution with
an optional centralized cloud storage, that can be used as a traditional cloud
storage.

See on: https://librevault.com

Usage:
  librevault-daemon [-v | -vv] [--data=<dir>]
  librevault-daemon (-h | --help)
  librevault-daemon --version

Options:
  --data=<dir>            set application data path

  -h --help               show this screen
  --version               show version
)";

static const char* BANNER =
    R"(   __    __ _                                _ __ )" "\n" \
    R"(  / /   /_/ /_  ____ _____ _  __ ___  __  __/ / /_)" "\n" \
    R"( / /   __/ /_ \/ ___/ ___/ / / / __ \/ / / / / __/)" "\n" \
    R"(/ /___/ / /_/ / /  / ___/\ \/ / /_/ / /_/ / / /___)" "\n" \
    R"(\____/_/\____/_/  /____/  \__/_/ /_/\____/_/\____/)" "\n";

// clang-format on

void spdlogMessageHandler(QtMsgType msg_type, const QMessageLogContext& ctx, const QString& msg) {
  auto logger = spdlog::get(Version::current().name().toStdString());
  if (!logger) return;

  QString message = QString(ctx.category) + " | " + msg;

  auto spdlog_level = spdlog::level::debug;

  switch (msg_type) {
    case QtDebugMsg:
      spdlog_level = spdlog::level::debug;
      break;
    case QtWarningMsg:
      spdlog_level = spdlog::level::warn;
      break;
    case QtCriticalMsg:
      spdlog_level = spdlog::level::err;
      break;
    case QtFatalMsg:
      spdlog_level = spdlog::level::critical;
      break;
    case QtInfoMsg:
      spdlog_level = spdlog::level::info;
      break;
  }

  logger->log(spdlog_level, message.toStdString());
  if (Q_UNLIKELY(msg_type == QtFatalMsg)) {
    logger->flush();
    abort();
  }
}

#ifdef Q_OS_UNIX
static void setupUnixSignalHandler() {
  struct sigaction sig;

  sig.sa_handler = Client::unixSignalHandler;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags |= SA_RESTART;

  sigaction(SIGTERM, &sig, nullptr);
  sigaction(SIGINT, &sig, nullptr);
}
#endif

int main(int argc, char** argv) {
  OPENSSL_init_ssl(OPENSSL_INIT_NO_ATEXIT, nullptr);
  do {
    // Argument parsing
    auto args =
        docopt::docopt(USAGE, {argv + 1, argv + argc}, true, librevault::Version().versionString().toStdString());

    // Initializing paths
    QString appdata_path;
    if (args["--data"].isString()) appdata_path = QString::fromStdString(args["--data"].asString());
    Paths::get(appdata_path);

    // Initializing log
    spdlog::level::level_enum log_level;
    qInfo() << args["-v"].asLong();
    switch (args["-v"].asLong()) {
      case 2:
        log_level = spdlog::level::trace;
        break;
      case 1:
        log_level = spdlog::level::debug;
        break;
      default:
        log_level = spdlog::level::info;
    }

    auto log = spdlog::get(Version::current().name().toStdString());
    if (!log) {
      std::vector<spdlog::sink_ptr> sinks{
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
          std::make_shared<spdlog::sinks::basic_file_sink_mt>(Paths::get()->log_path.toStdString())};

      log = std::make_shared<spdlog::logger>(Version::current().name().toStdString(), sinks.begin(), sinks.end());
      spdlog::register_logger(log);

      log->set_level(log_level);
    }

    // This overrides default Qt behavior, which is fine in many cases;
    qInstallMessageHandler(spdlogMessageHandler);

    // Initializing config
    Config::get();

    // Okay, that's a bit of fun, actually.
    std::cout << BANNER;
    qInfo() << Version::current().name() << Version::current().versionString();

    // And, run!
    auto client = std::make_unique<Client>(argc, argv);
#ifdef Q_OS_UNIX
    setupUnixSignalHandler();
#endif
    int ret = client->run();
    client.reset();

    qInfo() << "Clean shutdown";

    // Deinitialization
    log->flush();
    Config::deinit();
    Paths::deinit();

    if (ret != EXIT_RESTART) return ret;
  } while (true);

  return 0;
}
