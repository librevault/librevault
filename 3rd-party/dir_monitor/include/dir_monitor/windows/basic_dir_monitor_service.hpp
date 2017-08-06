//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "dir_monitor_impl.hpp"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <memory>
#include <string>
#include <stdexcept>
#include <windows.h>

namespace boost {
namespace asio {

namespace helper {

        inline void throw_system_error_if(bool condition, const std::string& msg)
        {
            if (condition)
            {
                DWORD last_error = GetLastError();
                boost::system::system_error e(boost::system::error_code(last_error, boost::system::get_system_category()), msg);
                boost::throw_exception(e);
            }
        }

        inline std::wstring to_utf16(const std::string& filename)
        {
            static std::wstring empty;
            if (filename.empty())
                return empty;

            int size = ::MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, NULL, 0);
            size = size > 0 ? size - 1 : 0;
            std::vector<wchar_t> buffer(size);
            helper::throw_system_error_if(::MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, &buffer[0], size), "boost::asio::basic_dir_monitor_service::to_utf16: MultiByteToWideChar failed");

            return std::wstring(buffer.begin(), buffer.end());
        }
}

template <typename DirMonitorImplementation = dir_monitor_impl>
class basic_dir_monitor_service
    : public boost::asio::io_service::service
{
public:
    struct completion_key
    {
        completion_key(HANDLE h, const std::wstring &d, std::shared_ptr<DirMonitorImplementation> &i)
            : handle(h),
            dirname(d),
            impl(i)
        {
            ZeroMemory(&overlapped, sizeof(overlapped));
        }

        HANDLE handle;
        std::wstring dirname;
        std::weak_ptr<DirMonitorImplementation> impl;
        char buffer[1024];
        OVERLAPPED overlapped;
    };

    static boost::asio::io_service::id id;

    explicit basic_dir_monitor_service(boost::asio::io_service &io_service)
        : boost::asio::io_service::service(io_service),
        last_work_thread_exception_ptr_(nullptr),
        iocp_(init_iocp()),
        run_(true),
        work_thread_(&boost::asio::basic_dir_monitor_service<DirMonitorImplementation>::work_thread, this),
        async_monitor_work_(std::make_unique<boost::asio::io_service::work>(async_monitor_io_service_)),
        async_monitor_thread_(boost::bind(&boost::asio::io_service::run, &async_monitor_io_service_))
    {
    }

    typedef std::shared_ptr<DirMonitorImplementation> implementation_type;

    void construct(implementation_type &impl)
    {
        impl = std::make_shared<DirMonitorImplementation>();
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
        std::wstring wdirname = helper::to_utf16(dirname);
        if (!boost::filesystem::is_directory(wdirname))
            throw std::invalid_argument("boost::asio::basic_dir_monitor_service::add_directory: " + dirname + " is not a valid directory entry");

        HANDLE handle = CreateFileW(wdirname.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
        helper::throw_system_error_if(INVALID_HANDLE_VALUE == handle, "boost::asio::basic_dir_monitor_service::add_directory: CreateFile failed");

        // a smart pointer is used to free allocated memory automatically in case of 
        // exceptions while handing over a completion key to the I/O completion port module,
        // the ownership has to be *released* at the end of scope so as not to free the memory
        // the OS kernel is using.
        auto ck_holder = std::make_unique<completion_key>(handle, wdirname, impl);
        helper::throw_system_error_if(NULL == CreateIoCompletionPort(ck_holder->handle, iocp_, reinterpret_cast<ULONG_PTR>(ck_holder.get()), 0),
            "boost::asio::basic_dir_monitor_service::add_directory: CreateIoCompletionPort failed");

        DWORD bytes_transferred; // ignored
        helper::throw_system_error_if(FALSE == ReadDirectoryChangesW(ck_holder->handle, ck_holder->buffer, sizeof(ck_holder->buffer), TRUE, 0x1FF, &bytes_transferred, &ck_holder->overlapped, NULL),
            "boost::asio::basic_dir_monitor_service::add_directory: ReadDirectoryChangesW failed");

        impl->add_directory(dirname, ck_holder->handle);

        // if we come all along here, surviving all possible exceptions, 
        // the allocated memory has been successfully handed over to the OS, 
        // and we should let go of the ownership.  
        ck_holder.release();
    }

    void remove_directory(implementation_type &impl, const std::string &dirname)
    {
        // Removing the directory from the implementation will automatically close the associated file handle.
        // Closing the file handle again will make GetQueuedCompletionStatus() return where the completion key
        // is then deleted.
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
            PostAndWait(ec, ev);
        }

    protected:
        void PostAndWait(const boost::system::error_code ec, const dir_monitor_event& ev) const
        {
            boost::mutex post_mutex;
            boost::condition_variable post_conditional_variable;
            bool post_cancel = false;

            this->io_service_.post(
                [&]
                {
                    handler_(ec, ev);
                    boost::unique_lock<boost::mutex> lock(post_mutex);
                    post_cancel = true;
                    post_conditional_variable.notify_one();
                }
            );
            boost::unique_lock<boost::mutex> lock(post_mutex);
            while (!post_cancel)
                post_conditional_variable.wait(lock);
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

        // The work thread is stopped and joined, too.
        stop_work_thread();
        work_thread_.join();

        CloseHandle(iocp_);
    }

    HANDLE init_iocp()
    {
        HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        helper::throw_system_error_if(iocp == NULL, "boost::asio::basic_dir_monitor_service::init_iocp: CreateIoCompletionPort failed");

        return iocp;
    }

    void work_thread()
    {
        while (running())
        {
            try
            {
                work();
            }
            catch (...)
            {
                last_work_thread_exception_ptr_ = std::current_exception();
                this->get_io_service().post(boost::bind(&boost::asio::basic_dir_monitor_service<DirMonitorImplementation>::throw_work_exception_handler, this));
            }
        }
    }

    void work()
    {
        DWORD bytes_transferred = 0;
        completion_key* ck = nullptr;
        OVERLAPPED *overlapped = nullptr;

        helper::throw_system_error_if(!GetQueuedCompletionStatus(iocp_, &bytes_transferred, reinterpret_cast<PULONG_PTR>(&ck), &overlapped, INFINITE),
            "boost::asio::basic_dir_monitor_service::work_thread: GetQueuedCompletionStatus failed");

        // a smart pointer is used to free allocated memory automatically in case of 
        // exceptions while handing over a completion key to the I/O completion port module,
        // the ownership has to be *released* at the end of scope so as not to free the memory
        // the OS kernel is using.
        std::unique_ptr<completion_key> ck_holder(ck);
        if (!ck_holder || !bytes_transferred)
            return;

        // If a file handle is closed GetQueuedCompletionStatus() returns and bytes_transferred will be set to 0.
        // The completion key must be deleted then as it won't be used anymore.

        // We must check if the implementation still exists. If the I/O object is destroyed while a directory event
        // is detected we have a race condition. Using a weak_ptr and a lock we make sure that we either grab a
        // shared_ptr first or - if the implementation has already been destroyed - don't do anything at all.
        implementation_type impl = ck_holder->impl.lock();

        // If the implementation doesn't exist anymore we must delete the completion key as it won't be used anymore.
        if (impl) 
        {
            DWORD offset = 0;
            PFILE_NOTIFY_INFORMATION fni = nullptr;
            do
            {
                fni = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(ck_holder->buffer + offset);
                dir_monitor_event::event_type type = dir_monitor_event::null;
                switch (fni->Action)
                {
                case FILE_ACTION_ADDED: type = dir_monitor_event::added; break;
                case FILE_ACTION_REMOVED: type = dir_monitor_event::removed; break;
                case FILE_ACTION_MODIFIED: type = dir_monitor_event::modified; break;
                case FILE_ACTION_RENAMED_OLD_NAME: type = dir_monitor_event::renamed_old_name; break;
                case FILE_ACTION_RENAMED_NEW_NAME: type = dir_monitor_event::renamed_new_name; break;
                }
                impl->pushback_event(dir_monitor_event(boost::filesystem::path(ck_holder->dirname) / std::wstring(fni->FileName, fni->FileNameLength / sizeof(WCHAR)), type));
                offset += fni->NextEntryOffset;
            } while (fni->NextEntryOffset);

            ZeroMemory(&ck_holder->overlapped, sizeof(ck_holder->overlapped));
            helper::throw_system_error_if(!ReadDirectoryChangesW(ck_holder->handle,ck_holder->buffer, sizeof(ck_holder->buffer), TRUE, 0x1FF, &bytes_transferred, &ck_holder->overlapped, NULL),
                "boost::asio::basic_dir_monitor_service::work_thread: ReadDirectoryChangesW failed");
        }
    
        // if we come all along here, surviving all possible exceptions, 
        // the allocated memory has been successfully handed over to the OS, 
        // and we should let go of the ownership.  
        ck_holder.release();
    }

    bool running()
    {
        // Access to run_ is sychronized with stop_work_thread().
        boost::mutex::scoped_lock lock(work_thread_mutex_);
        return run_;
    }

    void throw_work_exception_handler()
    {
        if (last_work_thread_exception_ptr_)
            std::rethrow_exception(last_work_thread_exception_ptr_);
    }

    void stop_work_thread()
    {
        // Access to run_ is sychronized with running().
        boost::mutex::scoped_lock lock(work_thread_mutex_);
        run_ = false;

        // By setting the third paramter to 0 GetQueuedCompletionStatus() will return with a null pointer as the completion key.
        // The work thread won't do anything except checking if it should continue to run. As run_ is set to false it will stop.
        helper::throw_system_error_if(!PostQueuedCompletionStatus(iocp_, 0, 0, NULL), "boost::asio::basic_dir_monitor_service::stop_work_thread: PostQueuedCompletionStatus failed");
    }

    std::exception_ptr last_work_thread_exception_ptr_;
    HANDLE iocp_;
    boost::mutex work_thread_mutex_;
    bool run_;
    boost::thread work_thread_;
    boost::asio::io_service async_monitor_io_service_;
    std::unique_ptr<boost::asio::io_service::work> async_monitor_work_;
    boost::thread async_monitor_thread_;
};

template <typename DirMonitorImplementation>
boost::asio::io_service::id basic_dir_monitor_service<DirMonitorImplementation>::id;

}
}

