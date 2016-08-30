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

#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/locale.hpp>

#include <stdio.h>
#include <locale>
#include <fstream>

namespace librevault {

#if BOOST_OS_WINDOWS
	using native_char_t = wchar_t;
#else
	using native_char_t = char;
#endif

inline FILE* native_fopen(const native_char_t* filepath, const char* mode) {
#if BOOST_OS_WINDOWS
	return _wfopen(filepath, boost::locale::conv::utf_to_utf<wchar_t>(mode).c_str());
#else
	return fopen(filepath, mode);
#endif
}

inline int cx_fileno(FILE *stream) {
#if BOOST_OS_WINDOWS
	return _fileno(stream);
#else
	return fileno(stream);
#endif
}

using fdstreambuf = boost::iostreams::stream_buffer<boost::iostreams::file_descriptor>;

class file_wrapper {
private:
	FILE* handle_ = nullptr;
	std::unique_ptr<fdstreambuf> buf_;
	std::unique_ptr<std::iostream> ios_;

public:
	inline file_wrapper() {}
	inline file_wrapper(const native_char_t* path, const char* mode) {
		open(path, mode);
	}
	inline file_wrapper(const boost::filesystem::path& path, const char* mode) {
		open(path, mode);
	}
	inline ~file_wrapper() {
		close();
	}

	inline std::iostream& ios() {
		return *ios_;
	}

	inline void open(const native_char_t* path, const char* mode) {
		handle_ = native_fopen(path, mode);
		if(handle_) {
			buf_ = std::make_unique<fdstreambuf>(cx_fileno(handle_), boost::iostreams::never_close_handle);
			ios_ = std::make_unique<std::iostream>(buf_.get());
		}else{
			ios_ = std::make_unique<std::fstream>();
		}
	}

	inline void open(const boost::filesystem::path& path, const char* mode) {
		open(path.native().c_str(), mode);
	}

	inline void close() {
		ios_.reset();
		buf_.reset();
		if(handle_)
			fclose(handle_);
		handle_ = nullptr;
	}
};

static void file_move(const boost::filesystem::path& from, const boost::filesystem::path& to) {
	boost::filesystem::remove(to);
	try {
		boost::filesystem::create_directories(to.parent_path());
		boost::filesystem::rename(from, to);
	}catch(boost::filesystem::filesystem_error& e){
		boost::filesystem::copy(from, to);
		boost::filesystem::remove(from);
	}
}

} /* namespace librevault */
