/* Copyright (C) 2016 Alexander Shishenko <GamePad64@gmail.com>
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

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include <stdio.h>
#include <locale>

namespace librevault {

#if BOOST_OS_WINDOWS
	using native_char_t = wchar_t;
#else
	using native_char_t = char;
#endif

inline FILE* native_fopen(const wchar_t* filepath, const char* mode) {
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
	FILE* handle_;
	std::unique_ptr<fdstreambuf> buf_;
	std::unique_ptr<std::iostream> ios_;

public:
	inline file_wrapper(const native_char_t* path, const char* mode) {
		handle_ = native_fopen(path, mode);
		buf_ = std::make_unique<fdstreambuf>(cx_fileno(handle_), boost::iostreams::never_close_handle);
		ios_ = std::make_unique<std::iostream>(buf_.get());
	}
	inline ~file_wrapper() {
		ios_.reset();
		buf_->close();
		buf_.reset();
		fclose(handle_);
		handle_ = nullptr;
	}

	inline std::iostream& ios() {
		return *ios_;
	}
};

} /* namespace librevault */
