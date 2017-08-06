//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "basic_dir_monitor.hpp"
#include <boost/predef/os.h>

#if BOOST_OS_WINDOWS
#  include "windows/basic_dir_monitor_service.hpp"
#elif (BOOST_OS_LINUX || BOOST_OS_ANDROID)
#  include "inotify/basic_dir_monitor_service.hpp"
#elif BOOST_OS_MACOS
#  include "fsevents/basic_dir_monitor_service.hpp"
#elif BOOST_OS_BSD
#  include "kqueue/basic_dir_monitor_service.hpp"
#else
#  error "Platform not supported."
#endif

namespace boost {
namespace asio {

typedef basic_dir_monitor<basic_dir_monitor_service<> > dir_monitor;

}
}

