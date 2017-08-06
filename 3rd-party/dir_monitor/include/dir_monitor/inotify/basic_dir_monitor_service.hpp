//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "dir_monitor_impl.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include <memory>
#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace boost {
namespace asio {

template <typename DirMonitorImplementation = dir_monitor_impl>
class basic_dir_monitor_service
    : public boost::asio::io_service::service
{
public:
    static boost::asio::io_service::id id;

    explicit basic_dir_monitor_service(boost::asio::io_service &io_service)
        : boost::asio::io_service::service(io_service),
        async_monitor_work_(new boost::asio::io_service::work(async_monitor_io_service_)),
        async_monitor_thread_(boost::bind(&boost::asio::io_service::run, &async_monitor_io_service_))
    {
    }

    typedef std::shared_ptr<DirMonitorImplementation> implementation_type;

    void construct(implementation_type &impl)
    {
        impl.reset(new DirMonitorImplementation());
        impl->begin_read();
    }

    void destroy(implementation_type &impl)
    {
        // If an asynchronous call is currently waiting for an event
        // we must interrupt the blocked call to make sure it returns.
        impl->destroy();

        impl.reset();
    }

    void add_directory(implementation_type &impl, const std::string &dirname)
    {
        if (!boost::filesystem::is_directory(dirname))
            throw std::invalid_argument("boost::asio::basic_dir_monitor_service::add_directory: " + dirname + " is not a valid directory entry");

        impl->add_directory(dirname);
    }

    void remove_directory(implementation_type &impl, const std::string &dirname)
    {
        impl->remove_directory(dirname);
    }

    dir_monitor_event monitor(implementation_type &impl, boost::system::error_code &ec)
    {
        return impl->popfront_event(ec);
    }

    template <typename Handler>
    class monitor_operation
    {
    public:
        monitor_operation(implementation_type &impl, boost::asio::io_service &io_service, Handler handler)
            : impl_(impl),
            io_service_(io_service),
            work_(io_service),
            handler_(handler)
        {
        }

        void operator()() const
        {
            implementation_type impl = impl_.lock();
            boost::system::error_code ec = boost::asio::error::operation_aborted;
            dir_monitor_event ev;
            if (impl)
                ev = impl->popfront_event(ec);
            if(ec != boost::asio::error::operation_aborted)
                this->io_service_.post(boost::asio::detail::bind_handler(handler_, ec, ev));
        }

    private:
        std::weak_ptr<DirMonitorImplementation> impl_;
        boost::asio::io_service &io_service_;
        boost::asio::io_service::work work_;
        Handler handler_;
    };

    template <typename Handler>
    void async_monitor(implementation_type &impl, Handler handler)
    {
        this->async_monitor_io_service_.post(monitor_operation<Handler>(impl, this->get_io_service(), handler));
    }

private:
    virtual void shutdown_service() override
    {
        // The async_monitor thread will finish when async_monitor_work_ is reset as all asynchronous
        // operations have been aborted and were discarded before (in destroy).
        async_monitor_work_.reset();

        // Event processing is stopped to discard queued operations.
        async_monitor_io_service_.stop();

        // The async_monitor thread is joined to make sure the directory monitor service is
        // destroyed _after_ the thread is finished (not that the thread tries to access
        // instance properties which don't exist anymore).
        async_monitor_thread_.join();
    }

    boost::asio::io_service async_monitor_io_service_;
    std::unique_ptr<boost::asio::io_service::work> async_monitor_work_;
    std::thread async_monitor_thread_;
};

template <typename DirMonitorImplementation>
boost::asio::io_service::id basic_dir_monitor_service<DirMonitorImplementation>::id;

}
}

