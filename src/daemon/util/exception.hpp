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
