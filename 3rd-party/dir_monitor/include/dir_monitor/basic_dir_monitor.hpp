//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <string>

namespace boost {
namespace asio {

struct dir_monitor_event
{
    enum event_type
    {
        null = 0,
        added = 1,
        removed = 2,
        modified = 3,
        renamed_old_name = 4,
        renamed_new_name = 5,
        /**
         * In some cases a recursive scan of directory under dirname is required.
         */
        recursive_rescan = 6
    };

    dir_monitor_event()
        : type(null) { }

    dir_monitor_event(const boost::filesystem::path &p, event_type t)
        : path(p), type(t) { }

    const char* type_cstr() const
    {
        switch(type) {
            case boost::asio::dir_monitor_event::added: return "ADDED";
            case boost::asio::dir_monitor_event::removed: return "REMOVED";
            case boost::asio::dir_monitor_event::modified: return "MODIFIED";
            case boost::asio::dir_monitor_event::renamed_old_name: return "RENAMED (OLD NAME)";
            case boost::asio::dir_monitor_event::renamed_new_name: return "RENAMED (NEW NAME)";
            case boost::asio::dir_monitor_event::recursive_rescan: return "RESCAN DIR";
            default: return "UNKNOWN";
        }
    }

    boost::filesystem::path path;
    event_type type;
};

inline std::ostream& operator << (std::ostream& os, dir_monitor_event const& ev)
{
    os << "dir_monitor_event " << ev.type_cstr() << " " << ev.path;
    return os;
}

template <typename Service>
class basic_dir_monitor
    : public boost::asio::basic_io_object<Service>
{
public:
    explicit basic_dir_monitor(boost::asio::io_service &io_service)
        : boost::asio::basic_io_object<Service>(io_service)
    {
    }

    void add_directory(const std::string &dirname)
    {
        this->get_service().add_directory(this->get_implementation(), dirname);
    }

    void remove_directory(const std::string &dirname)
    {
        this->get_service().remove_directory(this->get_implementation(), dirname);
    }

    dir_monitor_event monitor()
    {
        boost::system::error_code ec;
        dir_monitor_event ev = this->get_service().monitor(this->get_implementation(), ec);
        boost::asio::detail::throw_error(ec);
        return ev;
    }

    dir_monitor_event monitor(boost::system::error_code &ec)
    {
        return this->get_service().monitor(this->get_implementation(), ec);
    }

    template <typename Handler>
    void async_monitor(Handler handler)
    {
        this->get_service().async_monitor(this->get_implementation(), handler);
    }
};

}
}

