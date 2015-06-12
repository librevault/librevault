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
#include "Options.h"

namespace librevault {

ptree Options::defaults = ptree();
const char* Options::defaults_text =
"device_name \"\"	; Device name you choose for this instance of Librevault. Default value is the actual hostname of your computer\r\n"
"threads 0	; Number of CPU worker threads. 0 = number of hardware CPU threads\r\n"
"net\r\n"
"{\r\n"
"	ip \"0::0\"\r\n"
"	port 0\r\n"
"}\r\n"
"discovery\r\n"
"{\r\n"
"	multicast4\r\n"
"	{\r\n"
"		enable true\r\n"
"		multicast_ip \"239.192.152.144\"\r\n"
"		multicast_port 28914\r\n"
"		repeat_interval 30\r\n"
"	}\r\n"
"	multicast6\r\n"
"	{\r\n"
"		enable true\r\n"
"		multicast_ip \"ff08::BD02\"\r\n"
"		multicast_port 28914\r\n"
"		repeat_interval 30\r\n"
"	}\r\n"
"}\r\n"
"tor	; We support routing through Tor\r\n"
"{\r\n"
"	enable true\r\n"
"	tor_port 9050	; Port, where Tor proxy is listening\r\n"
"}\r\n"
;

} /* namespace librevault */
