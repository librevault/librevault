#pragma once
#include <QString>
#include <boost/filesystem/path.hpp>

namespace librevault {

inline QString conv_fspath(const boost::filesystem::path& path) { return QString::fromStdWString(path.wstring()); }

inline boost::filesystem::path conv_fspath(const QString& path) { return boost::filesystem::path(path.toStdWString()); }

}  // namespace librevault
