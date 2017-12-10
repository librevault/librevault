#include <boost/filesystem.hpp>
#include "dir_monitor/dir_monitor.hpp"

#define TEST_DIR1 "F95A7AE9-D5F5-459a-AB8D-28649FB1FF34"

boost::asio::io_service io_service;

#if defined(__APPLE__) && defined(__MACH__)
void print_fsevents_flags(unsigned flags)
{
    std::cout << "Flags " << [](unsigned flags) {
        std::ostringstream oss;
        if (flags & kFSEventStreamEventFlagMustScanSubDirs) oss << "MustScanSubDirs,";
        if (flags & kFSEventStreamEventFlagUserDropped) oss << "UserDropped,";
        if (flags & kFSEventStreamEventFlagKernelDropped) oss << "KernelDropped,";
        if (flags & kFSEventStreamEventFlagEventIdsWrapped) oss << "EventIdsWrapped,";
        if (flags & kFSEventStreamEventFlagHistoryDone) oss << "HistoryDone,";
        if (flags & kFSEventStreamEventFlagRootChanged) oss << "RootChanged,";
        if (flags & kFSEventStreamEventFlagMount) oss << "Mount,";
        if (flags & kFSEventStreamEventFlagUnmount) oss << "Unmount,";
        if (flags & kFSEventStreamEventFlagItemCreated) oss << "ItemCreated,";
        if (flags & kFSEventStreamEventFlagItemRemoved) oss << "ItemRemoved,";
        if (flags & kFSEventStreamEventFlagItemInodeMetaMod) oss << "ItemInodeMetaMod,";
        if (flags & kFSEventStreamEventFlagItemRenamed) oss << "ItemRenamed,";
        if (flags & kFSEventStreamEventFlagItemModified) oss << "ItemModified,";
        if (flags & kFSEventStreamEventFlagItemFinderInfoMod) oss << "ItemFinderInfoMod,";
        if (flags & kFSEventStreamEventFlagItemChangeOwner) oss << "ItemChangeOwner,";
        if (flags & kFSEventStreamEventFlagItemXattrMod) oss << "ItemXattrMod,";
        if (flags & kFSEventStreamEventFlagItemIsFile) oss << "ItemIsFile,";
        if (flags & kFSEventStreamEventFlagItemIsDir) oss << "ItemIsDir,";
        if (flags & kFSEventStreamEventFlagItemIsSymlink) oss << "ItemIsSymlink,";
        return oss.str();
    }(flags) << std::endl;
}
#endif


void event_handler(boost::asio::dir_monitor& dm, const boost::system::error_code &ec, const boost::asio::dir_monitor_event &ev)
{
    if (ec) {
        std::cout << "Error code " << ec << std::endl;
    } else {
        std::cout << ev << std::endl;
        // Keep it posted forever.
        dm.async_monitor([&](const boost::system::error_code &ec, const boost::asio::dir_monitor_event &ev) {
            event_handler(dm, ec, ev);
        });
    }
}

int main()
{
    boost::filesystem::create_directory(TEST_DIR1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);
    dm.async_monitor([&](const boost::system::error_code &ec, const boost::asio::dir_monitor_event &ev) {
        event_handler(dm, ec, ev);
    });

    boost::asio::io_service::work workload(io_service);
    io_service.run();
}
