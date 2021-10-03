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
#pragma once
#include <docopt/docopt.h>

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QUrl>

namespace librevault {

class CliApplication : public QCoreApplication {
 public:
  CliApplication(int argc, char** argv, const char* USAGE);

 private:
  std::map<std::string, docopt::value> args;
  QNetworkAccessManager* nam_;
  QUrl daemon_control_;

  void performProcessing();
  static QTextStream& qStdOut() {
    static QTextStream ts(stdout);
    return ts;
  }

  // API Actions
  void action_shutdown();
  void action_restart();
  void action_version();

  void action_globals_list();
  void action_globals_get();
  void action_globals_set();
  void action_globals_unset();
};

}  // namespace librevault
