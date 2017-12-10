#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/test/test_tools.hpp>

#include <string>
#include <vector>
#include <algorithm>

struct splited_path
{
    splited_path(const boost::filesystem::path& p)
        : m_path(split_path(p))
    {
    }

    splited_path(const char* p)
        : m_path(split_path(boost::filesystem::path(p)))
    {
    }

    friend bool operator==(const splited_path& lhs, const splited_path& rhs)
    {
        const bool revert = lhs.m_path.size() > rhs.m_path.size();
        const auto &x = revert ? rhs : lhs;
        const auto &y = revert ? lhs : rhs;
        if (!x.m_path.size())
            return false;

        const auto m = std::mismatch(x.m_path.rbegin(), x.m_path.rend(), y.m_path.rbegin());
        return (m.first == x.m_path.rend());
    }

    std::vector<std::string> split_path(const boost::filesystem::path& p)
    {
        std::vector<std::string> result;
        boost::split(result, p.generic_string(), boost::is_any_of("/"), boost::algorithm::token_compress_on);
        boost::trim_if(result, [](const std::string& str) { return str.empty(); });
        return result;
    }

private:
    std::vector<std::string> m_path;
};

struct the_same_paths_relative_impl
{
    boost::test_tools::predicate_result
    operator()(splited_path const& left, splited_path const& right) const
    {
        return left == right;
    }
};

//  Special macro for checking if two paths (which can be both absolute/relative) is the same "file" path
//
//  Examples of using:
//
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("d/b", "a/b");          =>  [x]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("/a/b", "a/b");         =>  [v]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("b", "a/b");            =>  [v]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("a", "a/b");            =>  [x]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("a/b", "c:/a/b");       =>  [v]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("c:/a/b", "a/b");       =>  [v]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("c:/a/b", "c:/a/b");    =>  [v]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("c:/a/b", "c:/a");      =>  [x]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("c:/a/b", "c:/a");      =>  [x]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("c:/a/b", "c:/d/b");    =>  [x]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("c:/a/b", "d:/a/b");    =>  [x]
//      BOOST_CHECK_THE_SAME_PATHS_RELATIVE("", "d:/a/b");          =>  [x]

#if (BOOST_VERSION >= 105900)
#define BOOST_CHECK_THE_SAME_PATHS_RELATIVE( L, R ) \
    BOOST_TEST_TOOL_IMPL( 0, ::the_same_paths_relative_impl(), "paths not the same", \
        CHECK, CHECK_EQUAL, (L)(R) )
#else
#define BOOST_CHECK_THE_SAME_PATHS_RELATIVE( L, R ) \
    BOOST_CHECK_WITH_ARGS_IMPL( the_same_paths_relative_impl(), "paths not the same", \
        CHECK, CHECK_EQUAL, (L)(R) )
#endif
