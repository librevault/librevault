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
#include <librevault/Secret.h>
#include <boost/locale.hpp>
#include <docopt.h>
#include <spdlog/spdlog.h>

using namespace librevault;	// This is allowed only because this is main.cpp file and it is extremely unlikely that this file will be included in any other file.

///////////////////////////////////////////////////////////////////////80 chars/
static const char* USAGE =
R"(Librevault command-line interface.

Librevault is an open source peer-to-peer file synchronization solution with
an optional centralized cloud storage, that can be used as a traditional cloud
storage.

See on: https://librevault.com

Usage:
  librevault [-v | -vv] [--data=<dir>]
  librevault gen-secret
  librevault derive <secret> <type>
  librevault (-h | --help)
  librevault --version

Commands:
  gen-secret              generate new Owner Secret (type A)
  derive <secret> <type>  derive Secret with less permissions from existing
                          (i.e. type C from A)

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
			logger->debug() << ctx.category << " | " << msg.toStdString();
			break;
		case QtWarningMsg:
			logger->warn() << ctx.category << " | " << msg.toStdString();
			break;
		case QtCriticalMsg:
			logger->critical() << ctx.category << " | " << msg.toStdString();
			break;
		case QtFatalMsg:
			logger->emerg() << ctx.category << " | " << msg.toStdString();
			logger->flush();
			abort();
		case QtInfoMsg:
			logger->info() << ctx.category << " | " << msg.toStdString();
			break;
		default:
			logger->info() << ctx.category << " | " << msg.toStdString();
	}
}

int main(int argc, char** argv) {
	do {
		// Global initialization
		std::locale::global(boost::locale::generator().generate(""));
		boost::filesystem::path::imbue(std::locale());

		// Argument parsing
		auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, librevault::Version().version_string().toStdString());

		// Initializing paths
		boost::filesystem::path appdata_path;
		if(args["--data"].isString())
			appdata_path = args["--data"].asString();
		Paths::get(appdata_path);

		// Initializing log
		spdlog::level::level_enum log_level;
		switch(args["-v"].asLong()) {
			case 2:     log_level = spdlog::level::trace; break;
			case 1:     log_level = spdlog::level::debug; break;
			default:    log_level = spdlog::level::info;
		}

		auto log = spdlog::get(Version::current().name().toStdString());
		if(!log){
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());

			boost::filesystem::path log_path = Paths::get()->log_path.toStdWString();
			sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
				(log_path.parent_path() / log_path.stem()).native(), // TODO: support filenames with multiple dots
				log_path.extension().native().substr(1), 10 * 1024 * 1024, 9));

			log = std::make_shared<spdlog::logger>(Version::current().name().toStdString(), sinks.begin(), sinks.end());
			spdlog::register_logger(log);

			log->set_level(log_level);
			log->set_pattern("%Y-%m-%d %T.%f %t %L | %v");
			log->flush_on(spdlog::level::warn);
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
		log->info() << Version::current().name().toStdString() << " " << Version::current().version_string().toStdString();

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
