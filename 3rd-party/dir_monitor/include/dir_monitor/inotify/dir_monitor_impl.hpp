//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/bimap.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <array>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <sys/inotify.h>
#include <errno.h>

#ifndef DIR_MONITOR_INOTIFY_MODIFICATION_MASK
#	define DIR_MONITOR_INOTIFY_MODIFICATION_MASK IN_CLOSE_WRITE
#endif

namespace boost {
namespace asio {

class dir_monitor_impl
{
public:
    dir_monitor_impl()
        : fd_(init_fd()),
        run_(true),
        inotify_work_(new boost::asio::io_service::work(inotify_io_service_)),
        inotify_work_thread_(boost::bind(&boost::asio::io_service::run, &inotify_io_service_)),
        stream_descriptor_(new boost::asio::posix::stream_descriptor(inotify_io_service_, fd_))
    {
    }

    void add_directory(const std::string &dirname)
    {
        int wd =  inotify_add_watch(fd_, dirname.c_str(), IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | DIR_MONITOR_INOTIFY_MODIFICATION_MASK);
        if (wd == -1)
        {
            boost::system::system_error e(boost::system::error_code(errno, boost::system::system_category()), "boost::asio::dir_monitor_impl::add_directory: inotify_add_watch failed");
            boost::throw_exception(e);
        }

        std::unique_lock<std::mutex> lock(watch_descriptors_mutex_);
        watch_descriptors_.insert(watch_descriptors_t::value_type(wd, dirname));
        lock.unlock();
        check_sub_directory(dirname, true);
    }

    void remove_directory(const std::string &dirname)
    {
        std::unique_lock<std::mutex> lock(watch_descriptors_mutex_);
        watch_descriptors_t::right_map::iterator it = watch_descriptors_.right.find(dirname);
        if (it != watch_descriptors_.right.end())
        {
            inotify_rm_watch(fd_, it->second);
            watch_descriptors_.right.erase(it);
            lock.unlock();
            check_sub_directory(dirname, false);
        }
    }

    void check_sub_directory(const std::string &dirname, bool add_sub_directory)
    {
        boost::filesystem::directory_iterator end;
        for (boost::filesystem::directory_iterator iter(dirname); iter != end; ++iter) {
            if (boost::filesystem::is_directory(*iter)) {
                if (add_sub_directory) {
                    try {
                        add_directory((*iter).path().string());
                    } catch (const std::exception&) {
                        continue;
                    }
                } else {
                    remove_directory((*iter).path().string());
                }
            }
        }
    }

    void destroy()
    {
        inotify_work_.reset();
        inotify_io_service_.stop();
        inotify_work_thread_.join();
        stream_descriptor_.reset();

        std::unique_lock<std::mutex> lock(events_mutex_);
        run_ = false;
        events_cond_.notify_all();
    }

    dir_monitor_event popfront_event(boost::system::error_code &ec)
    {
        std::unique_lock<std::mutex> lock(events_mutex_);
        events_cond_.wait(lock, [&]() { return !(run_ && events_.empty()); });
        
        dir_monitor_event ev;
        ec = boost::system::error_code();
        if (!run_)
            ec = boost::asio::error::operation_aborted;
        else if (!events_.empty())
        {
            ev = events_.front();
            events_.pop_front();
        }
            
        return ev;
    }

    void pushback_event(dir_monitor_event ev)
    {
        std::unique_lock<std::mutex> lock(events_mutex_);
        if (run_)
        {
            events_.push_back(ev);
            events_cond_.notify_all();
        }
    }

private:
    int init_fd()
    {
        int fd = inotify_init();
        if (fd == -1)
        {
            boost::system::system_error e(boost::system::error_code(errno, boost::system::system_category()), "boost::asio::dir_monitor_impl::init_fd: init_inotify failed");
            boost::throw_exception(e);
        }
        return fd;
    }

public:
    void begin_read()
    {
        stream_descriptor_->async_read_some(boost::asio::buffer(read_buffer_),
            boost::bind(&dir_monitor_impl::end_read, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

private:
    void end_read(const boost::system::error_code &ec, std::size_t bytes_transferred)
    {
        if (!ec)
        {
            pending_read_buffer_ += std::string(read_buffer_.data(), bytes_transferred);
            while (pending_read_buffer_.size() > sizeof(inotify_event))
            {
                const inotify_event *iev = reinterpret_cast<const inotify_event*>(pending_read_buffer_.data());
                dir_monitor_event::event_type type = dir_monitor_event::null;
                switch (iev->mask)
                {
                case IN_CREATE: type = dir_monitor_event::added; break;
                case IN_DELETE: type = dir_monitor_event::removed; break;
                case IN_MOVED_FROM: type = dir_monitor_event::renamed_old_name; break;
                case IN_MOVED_TO: type = dir_monitor_event::renamed_new_name; break;
                case IN_MODIFY:
                case IN_CLOSE_WRITE: type = dir_monitor_event::modified; break;
                case IN_CREATE | IN_ISDIR:
                    {
                        type = dir_monitor_event::added;
                        add_directory(get_dirname(iev->wd) + "/" + iev->name);
                        break;
                    }
                }
                pushback_event(dir_monitor_event(boost::filesystem::path(get_dirname(iev->wd)) / iev->name, type));
                pending_read_buffer_.erase(0, sizeof(inotify_event) + iev->len);
            }

            begin_read();
        }
        else if (ec != boost::asio::error::operation_aborted)
        {
            boost::system::system_error e(ec);
            boost::throw_exception(e);
        }
    }

    std::string get_dirname(int wd)
    {
        std::unique_lock<std::mutex> lock(watch_descriptors_mutex_);
        watch_descriptors_t::left_map::iterator it = watch_descriptors_.left.find(wd);
        return it != watch_descriptors_.left.end() ? it->second : "";
    }

    int fd_;
    bool run_;
    boost::asio::io_service inotify_io_service_;
    std::unique_ptr<boost::asio::io_service::work> inotify_work_;
    std::thread inotify_work_thread_;
    
    std::unique_ptr<boost::asio::posix::stream_descriptor> stream_descriptor_;
    std::array<char, 4096> read_buffer_;
    std::string pending_read_buffer_;
    std::mutex watch_descriptors_mutex_;
    typedef boost::bimap<int, std::string> watch_descriptors_t;
    watch_descriptors_t watch_descriptors_;
    std::mutex events_mutex_;
    std::condition_variable events_cond_;
    std::deque<dir_monitor_event> events_;
};

}
}

