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
#include "PathNormalizer.h"
#include "control/FolderParams.h"
#include "util/fs.h"
#include "util/make_relpath.h"
#include <boost/locale.hpp>
#include <codecvt>

namespace librevault {

PathNormalizer::PathNormalizer(const FolderParams& params) : params_(params) {}

std::string PathNormalizer::normalize_path(const fs::path& abspath) const {
#ifdef LV_DEBUG_NORMALIZATION
	LOGFUNC() << abspath;
#endif

	// Relative path in platform-independent format
	fs::path rel_path = ::librevault::make_relpath(abspath, params_.path);

	std::string normpath = rel_path.generic_string(std::codecvt_utf8_utf16<wchar_t>());
	if(params_.normalize_unicode)	// Unicode normalization NFC (for compatibility)
		normpath = boost::locale::normalize(normpath, boost::locale::norm_nfc);

	// Removing last '/' in directories
	if(normpath.size() > 0 && normpath.back() == '/')
		normpath.pop_back();

#ifdef LV_DEBUG_NORMALIZATION
	LOGFUNC() << normpath;
#endif
	return normpath;
}

fs::path PathNormalizer::absolute_path(const std::string& normpath) const {
#ifdef LV_DEBUG_NORMALIZATION
	LOGFUNC() << normpath;
#endif

#if BOOST_OS_WINDOWS
	std::wstring osnormpath = boost::locale::conv::utf_to_utf<wchar_t>(normpath);
#elif BOOST_OS_MACOS
	std::string osnormpath = boost::locale::normalize(normpath, boost::locale::norm_nfd);
#else
	const std::string& osnormpath(normpath);
#endif
	fs::path abspath = params_.path / osnormpath;

#ifdef LV_DEBUG_NORMALIZATION
	LOGFUNC() << abspath;
#endif
	return abspath;
}

} /* namespace librevault */
