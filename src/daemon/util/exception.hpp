/* Copyright (C) 2017 Alexander Shishenko <alex@shishenko.com>
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
#include <QString>
#include <stdexcept>
#include <string>

#define DECLARE_EXCEPTION_DETAIL(exception_name, parent_exception, exception_string)     \
  struct exception_name : public parent_exception {                                      \
    exception_name() : parent_exception(exception_string) {}                             \
    explicit exception_name(const char* what) : parent_exception(what) {}                \
    explicit exception_name(const std::string& what) : exception_name(what.c_str()) {}   \
    explicit exception_name(const QString& what) : exception_name(what.toStdString()) {} \
  }

#define DECLARE_EXCEPTION(exception_name, exception_string) \
  DECLARE_EXCEPTION_DETAIL(exception_name, std::runtime_error, exception_string)
