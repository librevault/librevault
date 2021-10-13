# Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# OS detection flags
if(WIN32)
  set(OS_WIN TRUE)
  set(OS_WINDOWS TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS_LINUX TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(OS_MAC TRUE)
endif()

# Bitness detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OS_64bit TRUE)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(OS_32bit TRUE)
endif()