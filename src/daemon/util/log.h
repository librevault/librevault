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
#include <QLatin1String>
#include <QLoggingCategory>
#include <QtDebug>
#include <boost/current_function.hpp>
#include <boost/preprocessor/cat.hpp>

#define LOG_SCOPE(SCOPE) \
  inline QString log_tag() const { return QStringLiteral(SCOPE); }

#define LOGD(ARGS) qDebug() << log_tag().toStdString().c_str() << "|" << ARGS

#define LOGI(ARGS) qInfo() << log_tag().toStdString().c_str() << "|" << ARGS

#define LOGW(ARGS) qWarning() << log_tag().toStdString().c_str() << "|" << ARGS

#define SCOPELOG_CLASS BOOST_PP_CAT(ScopeLog, __LINE__)
#define SCOPELOG(category)                                                    \
  class SCOPELOG_CLASS {                                                      \
    QLatin1String scope_;                                                     \
                                                                              \
   public:                                                                    \
    explicit SCOPELOG_CLASS(QLatin1String scope) : scope_(std::move(scope)) { \
      qCDebug(category).noquote() << "=>" << scope_;                      \
    }                                                                         \
    ~SCOPELOG_CLASS() { qCDebug(category).noquote() << "<=" << scope_; }  \
  };                                                                          \
  SCOPELOG_CLASS scopelog(QLatin1String(BOOST_CURRENT_FUNCTION))

#define LOGFUNC() qDebug() << BOOST_CURRENT_FUNCTION

#define LOGFUNCEND() qDebug() << "~" << BOOST_CURRENT_FUNCTION
