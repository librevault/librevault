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
#include "CliApplication.h"

using namespace librevault;	// This is allowed only because this is main.cpp file and it is extremely unlikely that this file will be included in any other file.

///////////////////////////////////////////////////////////////////////80 chars/
static const char* USAGE =
R"(Librevault command-line interface.

Librevault is an open source peer-to-peer file synchronization
solution with an optional centralized cloud storage, that can be used as a traditional cloud storage.

See on: https://librevault.com
GitHub: https://github.com/Librevault/librevault

Usage:
  librevault secret generate
  librevault secret derive <secret> <type>
  librevault secret folderid <secret>
  librevault shutdown [--daemon=<daemon>]
  librevault restart [--daemon=<daemon>]
  librevault version [--daemon=<daemon>]
  librevault globals list [--daemon=<daemon>]
  librevault globals get [--daemon=<daemon>] <key>
  librevault globals set [--daemon=<daemon>] <key> <value>
  librevault globals unset [--daemon=<daemon>] <key>
  librevault folder add [--daemon=<daemon>] <secret> <path> [(-o <key> <value>)]...
  librevault folder remove [--daemon=<daemon>] <folderid>
  librevault folder reindex [--daemon=<daemon>] <folderid>
  librevault folder list-folders [--daemon=<daemon>]
  librevault (-h | --help)

Commands:
  secret generate    generate new Owner Secret.
  secret derive      derive Secret of specified <type> from <secret>
  secret folderid    compute folder id (aka folder hash), which is used to send commands to folders
  shutdown           shutdown the daemon
  restart            restart the daemon
  version            get version of the daemon
  folder add         add synchronization folder
  folder remove      remove synchronization folder. <folderid> is a folder id, computed using "gen-folderid" command
  folder list        list all synchronization folders

Options:
  --daemon=<daemon>  set URL to Librevault Client API

  -o

  -h --help          show this screen
)";

int main(int argc, char** argv) {
	CliApplication app(argc, argv, USAGE);
	return app.exec();
}
