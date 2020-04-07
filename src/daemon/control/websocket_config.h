/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#pragma once
#include <websocketpp/config/asio.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/server.hpp>

namespace librevault {

/// Server config with asio transport and TLS enabled
struct asio_tls : public websocketpp::config::core {
  using type = asio_tls;
  using base = websocketpp::config::core;

  using concurrency_type = base::concurrency_type;

  using request_type = base::request_type;
  using response_type = base::response_type;

  using message_type = base::message_type;
  using con_msg_manager_type = base::con_msg_manager_type;
  using endpoint_msg_manager_type = base::endpoint_msg_manager_type;

#ifndef LV_DEBUG_WEBSOCKETPP
  using alog_type = websocketpp::log::stub;
  using elog_type = websocketpp::log::stub;
#else
  using alog_type = base::alog_type;
  using elog_type = base::elog_type;
#endif

  using rng_type = base::rng_type;

  struct transport_config : public base::transport_config {
    using concurrency_type = type::concurrency_type;
#ifndef LV_DEBUG_WEBSOCKETPP
    using alog_type = websocketpp::log::stub;
    using elog_type = websocketpp::log::stub;
#else
    using alog_type = type::alog_type;
    using elog_type = type::elog_type;
#endif
    using request_type = type::request_type;
    using response_type = type::response_type;
    using socket_type = websocketpp::transport::asio::tls_socket::endpoint;
  };

  using transport_type = websocketpp::transport::asio::endpoint<transport_config>;
};

/// Server config with asio transport and TLS disabled
struct asio_notls : public websocketpp::config::core {
  using type = asio_notls;
  using base = websocketpp::config::core;

  using concurrency_type = base::concurrency_type;

  using request_type = base::request_type;
  using response_type = base::response_type;

  using message_type = base::message_type;
  using con_msg_manager_type = base::con_msg_manager_type;
  using endpoint_msg_manager_type = base::endpoint_msg_manager_type;

  using alog_type = websocketpp::log::stub;
  using elog_type = websocketpp::log::stub;

  using rng_type = base::rng_type;

  struct transport_config : public base::transport_config {
    using concurrency_type = type::concurrency_type;
    using alog_type = websocketpp::log::stub;
    using elog_type = websocketpp::log::stub;
    using request_type = type::request_type;
    using response_type = type::response_type;
    using socket_type = websocketpp::transport::asio::basic_socket::endpoint;
  };

  using transport_type = websocketpp::transport::asio::endpoint<transport_config>;
};

}  // namespace librevault
