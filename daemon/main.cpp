/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#include "pch.h"
#include "Client.h"
#include "Version.h"
#include "control/Config.h"
#include <librevault/Secret.h>
#include <docopt.h>

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
  librevault gen-config
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

int main(int argc, char** argv) {
	auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, librevault::Version().version_string());

	if(args["gen-secret"].asBool()) {
		Secret s;
		std::cout << s;
		return 0;
	}

	if(args["derive"].asBool()) {
		Secret::Type type = (Secret::Type)args["<type>"].asString().at(0);
		std::cout << type << std::endl;
		Secret s(args["<secret>"].asString());
		std::cout << s.derive(type);
		return 0;
	}

	// Initializing config
	fs::path appdata_path;
	if(args["--data"].isString())
		appdata_path = args["--data"].asString();
	Config::init(appdata_path);

	if(args["gen-config"].asBool()) {
		std::cout << Config::get()->client_defaults().toStyledString();
		return 0;
	}


	// Okay, that's a bit of fun, actually.
	std::cout
		<< R"(   __    __ _                                _ __ )" << std::endl
		<< R"(  / /   /_/ /_  ____ _____ _  __ ___  __  __/ / /_)" << std::endl
		<< R"( / /   __/ /_ \/ ___/ ___/ / / / __ \/ / / / / __/)" << std::endl
		<< R"(/ /___/ / /_/ / /  / ___/\ \/ / /_/ / /_/ / / /___)" << std::endl
		<< R"(\____/_/\____/_/  /____/  \__/_/ /_/\____/_/\____/)" << std::endl;

	Client client(args);
	client.run();

	return 0;
}
