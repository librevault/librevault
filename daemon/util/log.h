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
#pragma once
#include "log_scope.h"
#include "Version.h"
#include <spdlog/spdlog.h>

#define LOGGER spdlog::get(Version::current().name().toStdString())

#define LOGT(ARGS) \
LOGGER->trace() << log_tag() << ARGS

#define LOGD(ARGS) \
LOGGER->debug() << log_tag() << ARGS

#define LOGI(ARGS) \
LOGGER->info() << log_tag() << ARGS

#define LOGN(ARGS) \
LOGGER->notice() << log_tag() << ARGS

#define LOGW(ARGS) \
LOGGER->warn() << log_tag() << ARGS

#define LOGE(ARGS) \
LOGGER->error() << log_tag() << ARGS

#define LOGC(ARGS) \
LOGGER->critical() << log_tag() << ARGS

#define LOGA(ARGS) \
LOGGER->alert() << log_tag() << ARGS

#define LOGEM(ARGS) \
LOGGER->emerg() << log_tag() << ARGS

#define LOGFUNC() \
LOGT(BOOST_CURRENT_FUNCTION) << " "

#define LOGFUNCEND() \
LOGT(BOOST_CURRENT_FUNCTION << " -- end ")
