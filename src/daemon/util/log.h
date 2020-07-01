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
      qCDebug(category).noquote() << "➡️" << scope_;                      \
    }                                                                         \
    ~SCOPELOG_CLASS() { qCDebug(category).noquote() << "⬅️" << scope_; }  \
  };                                                                          \
  SCOPELOG_CLASS scopelog(QLatin1String(BOOST_CURRENT_FUNCTION))

#define LOGFUNC() qDebug() << BOOST_CURRENT_FUNCTION

#define LOGFUNCEND() qDebug() << "~" << BOOST_CURRENT_FUNCTION
