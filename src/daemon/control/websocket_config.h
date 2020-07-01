#pragma once
#include <websocketpp/config/asio.hpp>
#include <websocketpp/logger/stub.hpp>
#include <websocketpp/server.hpp>

namespace librevault {

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
