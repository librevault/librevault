//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>
#include <boost/filesystem.hpp>
#include "dir_monitor/dir_monitor.hpp"
#include "check_paths.hpp"
#include "directory.hpp"

boost::asio::io_service io_service;

BOOST_AUTO_TEST_CASE(create_file)
{
    directory dir(TEST_DIR1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);

    auto test_file1 = dir.create_file(TEST_FILE1);

    boost::asio::dir_monitor_event ev = dm.monitor();

    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file1);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::added);
}

BOOST_AUTO_TEST_CASE(rename_file)
{
    directory dir(TEST_DIR1);
    auto test_file1 = dir.create_file(TEST_FILE1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);

    auto test_file2 = dir.rename_file(TEST_FILE1, TEST_FILE2);

    boost::asio::dir_monitor_event ev = dm.monitor();

    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file1);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::renamed_old_name);

    ev = dm.monitor();

    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file2);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::renamed_new_name);
}

BOOST_AUTO_TEST_CASE(remove_file)
{
    directory dir(TEST_DIR1);
    auto test_file1 = dir.create_file(TEST_FILE1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);

    dir.remove_file(TEST_FILE1);

    boost::asio::dir_monitor_event ev = dm.monitor();

    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file1);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::removed);
}

BOOST_AUTO_TEST_CASE(modify_file)
{
    directory dir(TEST_DIR1);
    auto test_file1 = dir.create_file(TEST_FILE1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);

    dir.write_file(TEST_FILE1, TEST_FILE2);

    boost::asio::dir_monitor_event ev = dm.monitor();

    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file1);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::modified);
}

BOOST_AUTO_TEST_CASE(multiple_events)
{
    directory dir(TEST_DIR1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);

    auto test_file1 = dir.create_file(TEST_FILE1);
    auto test_file2 = dir.rename_file(TEST_FILE1, TEST_FILE2);
    dir.remove_file(TEST_FILE2);

    boost::asio::dir_monitor_event ev = dm.monitor();
    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file1);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::added);

    ev = dm.monitor();
    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file1);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::renamed_old_name);

    ev = dm.monitor();
    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file2);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::renamed_new_name);

    ev = dm.monitor();
    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file2);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::removed);
}

BOOST_AUTO_TEST_CASE(dir_monitor_destruction)
{
    directory dir(TEST_DIR1);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(TEST_DIR1);

    dir.create_file(TEST_FILE1);
}

BOOST_AUTO_TEST_CASE(non_ascii_paths)
{
    char utf8DirName[]  = "\xe6\x97\xa5\xe6\x9c\xac\xe5\x9b\xbd"; // 日本国
    char utf8FileName[] = "\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9"".txt"; // العربية.txt

    directory dir(utf8DirName);

    boost::asio::dir_monitor dm(io_service);
    dm.add_directory(utf8DirName);

    auto test_file = dir.create_file(utf8FileName);

    boost::asio::dir_monitor_event ev = dm.monitor();
    BOOST_CHECK_THE_SAME_PATHS_RELATIVE(ev.path, test_file);
    BOOST_CHECK_EQUAL(ev.type, boost::asio::dir_monitor_event::added);
}
