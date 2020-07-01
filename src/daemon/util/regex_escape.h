#pragma once
#include <QString>

namespace librevault {

inline QString regex_escape(QString escaped) {
  escaped.replace("\\", "\\\\");
  escaped.replace("^", "\\^");
  escaped.replace(".", "\\.");
  escaped.replace("$", "\\$");
  escaped.replace("|", "\\|");
  escaped.replace("(", "\\(");
  escaped.replace(")", "\\)");
  escaped.replace("[", "\\[");
  escaped.replace("]", "\\]");
  escaped.replace("*", "\\*");
  escaped.replace("+", "\\+");
  escaped.replace("?", "\\?");
  escaped.replace("/", "\\/");

  return escaped;
}

}  // namespace librevault
