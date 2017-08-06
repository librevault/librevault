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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "Client.h"
#include "Version.h"
#include "control/Config.h"
#include "control/Paths.h"
#include <docopt.h>
#include "Secret.h"
#include <spdlog/spdlog.h>
#include <boost/filesystem/path.hpp>
#include <QDocopt.hpp>

using namespace librevault;	// This is allowed only because this is main.cpp file and it is extremely unlikely that this file will be included in any other file.

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

void spdlogMessageHandler(QtMsgType msg_type, const QMessageLogContext& ctx, const QString& msg) {
	auto logger = spdlog::get(Version::current().name().toStdString());
	if(!logger) return;

	switch(msg_type) {
		case QtDebugMsg:
			logger->debug(std::string(ctx.category) + " | " + msg.toStdString());
			break;
		case QtWarningMsg:
			logger->warn(std::string(ctx.category) + " | " + msg.toStdString());
			break;
		case QtCriticalMsg:
			logger->error(std::string(ctx.category) + " | " + msg.toStdString());
			break;
		case QtFatalMsg:
			logger->critical(std::string(ctx.category) + " | " + msg.toStdString());
			logger->flush();
			abort();
		case QtInfoMsg:
			logger->info(std::string(ctx.category) + " | " + msg.toStdString());
			break;
		default:
			logger->info(std::string(ctx.category) + " | " + msg.toStdString());
	}
}

int main(int argc, char** argv) {
  // Argument parsing
	QVariantMap args = qdocopt(USAGE, argc, argv, true, librevault::Version().version_string());

  // Initializing log verbosity
  spdlog::level::level_enum log_level;
  switch(args["-v"].toLongLong()) {
    case 2:     log_level = spdlog::level::trace; break;
    case 1:     log_level = spdlog::level::debug; break;
    default:    log_level = spdlog::level::info;
  }

	do {
		// Initializing paths
		QString appdata_path;
    if(args.contains("--data"))
      appdata_path = args["--data"].toString();
		Paths::get(appdata_path);

    // Initializing log
		auto log = spdlog::get(Version::current().name().toStdString());
		if(!log){
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());

			boost::filesystem::path log_path = Paths::get()->log_path.toStdWString();
			sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>(log_path.native()));

			log = std::make_shared<spdlog::logger>(Version::current().name().toStdString(), sinks.begin(), sinks.end());
			spdlog::register_logger(log);

			log->set_level(log_level);
			log->set_pattern("%Y-%m-%d %T.%f %t %L | %v");
		}

		// This overrides default Qt behavior, which is fine in many cases;
		qInstallMessageHandler(spdlogMessageHandler);

		// Initializing config
		Config::get();

		// Okay, that's a bit of fun, actually.
		std::cout
			<< R"(   __    __ _                                _ __ )" << std::endl
			<< R"(  / /   /_/ /_  ____ _____ _  __ ___  __  __/ / /_)" << std::endl
			<< R"( / /   __/ /_ \/ ___/ ___/ / / / __ \/ / / / / __/)" << std::endl
			<< R"(/ /___/ / /_/ / /  / ___/\ \/ / /_/ / /_/ / / /___)" << std::endl
			<< R"(\____/_/\____/_/  /____/  \__/_/ /_/\____/_/\____/)" << std::endl;
		log->info(Version::current().name().toStdString() + " " + Version::current().version_string().toStdString());

		// And, run!
		auto client = std::make_unique<Client>(argc, argv);
		int ret = client->run();
		client.reset();

		// Deinitialization
		log->flush();
		Config::deinit();
		Paths::deinit();

		if(ret != EXIT_RESTART) return ret;
	}while(true);

	return 0;
}
