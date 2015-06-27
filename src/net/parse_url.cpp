/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "parse_url.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

namespace librevault {

url parse_url(std::string url_str){
	boost::algorithm::trim(url_str);
	url parsed_url;

	std::string::const_iterator it_scheme_begin, it_scheme_end, it_userinfo_begin, it_userinfo_end, it_host_begin, it_host_end, it_port_begin, it_port_end;
	std::string::const_iterator it_authority_begin, it_authority_end;

	// Scheme section
	it_scheme_begin = url_str.cbegin();
	auto schema_end_pos = url_str.find("://", 0);
	if(schema_end_pos != std::string::npos)
		it_scheme_end = url_str.cbegin()+schema_end_pos;
	else
		it_scheme_end = it_scheme_begin;
	parsed_url.scheme.assign(it_scheme_begin, it_scheme_end);

	// Authority section
	it_authority_begin = it_scheme_end+3;
	it_authority_end = std::find(it_authority_begin, url_str.cend(), '/');

	// Authority->Userinfo section
	it_userinfo_begin = it_authority_begin;
	it_userinfo_end = std::find(it_userinfo_begin, it_authority_end, '@');
	if(it_userinfo_end != it_authority_end){
		it_host_begin = it_userinfo_end+1;
	}else{
		it_userinfo_end = it_userinfo_begin;
		it_host_begin = it_userinfo_end;
	}
	parsed_url.userinfo.assign(it_userinfo_end, it_userinfo_end);

	// Authority->Host section
	if(*it_host_begin == '['){	// We have IPv6 address.
		it_host_end = std::find(it_host_begin, it_authority_end, ']')+1;
	}else{
		it_host_end = std::find(it_host_begin, it_authority_end, ':');
	}
	parsed_url.host.assign(it_host_begin, it_host_end);

	// Authority->Port section
	it_port_begin = std::find(it_host_end, it_authority_end, ':')+1;
	it_port_end = it_authority_end;
	parsed_url.port = boost::lexical_cast<uint16_t>(&*it_port_begin, std::distance(it_port_begin, it_port_end));

	return parsed_url;
}

} /* namespace librevault */
