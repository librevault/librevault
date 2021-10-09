/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <docopt/docopt.h>
#include <QtGlobal>
#ifdef Q_OS_UNIX
#include <csignal>
#endif

#include <librevaultrs.hpp>
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

void rustMessageHandler(QtMsgType msg_type, const QMessageLogContext& ctx, const QString& msg) {
  auto category = QString(ctx.category).toUtf8();
  auto msg_utf8 = msg.toUtf8();

  auto level = Level::Debug;
  switch (msg_type) {
    case QtDebugMsg:
      level = Level::Debug;
      break;
    case QtWarningMsg:
      level = Level::Warn;
      break;
    case QtCriticalMsg:
    case QtFatalMsg:
      level = Level::Error;
      break;
    case QtInfoMsg:
      level = Level::Info;
      break;
  }

  log_message(level, {reinterpret_cast<const uint8_t*>(msg_utf8.data()), static_cast<uintptr_t>(msg_utf8.size())},
              {reinterpret_cast<const uint8_t*>(category.data()), static_cast<uintptr_t>(category.size())});

  if (Q_UNLIKELY(msg_type == QtFatalMsg)) {
    // flush logger
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
  do {
    // Argument parsing
    auto args =
        docopt::docopt(USAGE, {argv + 1, argv + argc}, true, librevault::Version().versionString().toStdString());

    // Initializing paths
    QString appdata_path;
    if (args["--data"].isString()) appdata_path = QString::fromStdString(args["--data"].asString());
    Paths::get(appdata_path);

    log_init();
    // This overrides default Qt behavior, which is fine in many cases;
    qInstallMessageHandler(rustMessageHandler);

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

    // Deinitialization
    Config::deinit();
    Paths::deinit();

    if (ret != EXIT_RESTART) return ret;
  } while (true);

  return 0;
}
