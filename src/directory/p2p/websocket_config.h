/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
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
 */
#include "../../pch.h"
#include "../../util/Loggable.h"
#pragma once

namespace librevault {

/// Client config with asio transport and TLS enabled, but with logging disabled
struct asio_tls_client : public websocketpp::config::core_client {
	using type = asio_tls_client;
	using base = websocketpp::config::core_client;

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
		using socket_type = websocketpp::transport::asio::tls_socket::endpoint;
	};

	using transport_type = websocketpp::transport::asio::endpoint<transport_config>;
};

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

	using alog_type = websocketpp::log::stub;
	using elog_type = websocketpp::log::stub;

	using rng_type = base::rng_type;

	struct transport_config : public base::transport_config {
		using concurrency_type = type::concurrency_type;
		using alog_type = websocketpp::log::stub;
		using elog_type = websocketpp::log::stub;
		using request_type = type::request_type;
		using response_type = type::response_type;
		using socket_type = websocketpp::transport::asio::tls_socket::endpoint;
	};

	using transport_type = websocketpp::transport::asio::endpoint<transport_config>;
};

} /* namespace librevault */
