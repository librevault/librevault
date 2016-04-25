# - find Sqlite 3
# SQLITE3_INCLUDE_DIR - Where to find Sqlite 3 header files (directory)
# SQLITE3_LIBRARY - Where the release library is
# SQLITE3_FOUND - Set to TRUE if we found everything (library, includes and executable)
# SQLITE3_VERSION - Sqlite3 version

# Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

find_library(SQLITE3_LIBRARY NAMES sqlite3)
find_path(SQLITE3_INCLUDE_DIR sqlite3.h)

# SQLite version
file(STRINGS "${SQLITE3_INCLUDE_DIR}/sqlite3.h" SQLITE3_HEADER_LINES REGEX "#define SQLITE_VERSION")
string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" SQLITE3_VERSION ${SQLITE3_HEADER_LINES})
unset(SQLITE3_HEADER_LINES)

find_package_handle_standard_args(Sqlite3
	REQUIRED_VARS SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR
	VERSION_VAR SQLITE3_VERSION
	)
