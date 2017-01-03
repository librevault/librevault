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
#include "P2PFolder.h"
#include "WSService.h"
#include "control/Config.h"
#include "folder/FolderGroup.h"
#include "folder/FolderService.h"
#include "nodekey/NodeKey.h"

#include <librevault/Tokens.h>

namespace librevault {

P2PFolder::P2PFolder(P2PProvider& provider, WSService& ws_service, NodeKey& node_key, FolderService& folder_service, WSService::connection conn, io_service& ios) :
		conn_(std::move(conn)),
		provider_(provider),
		ws_service_(ws_service),
		node_key_(node_key),
		ping_timer_(ios),
		timeout_timer_(ios) {
	std::ostringstream os; os << conn_.remote_endpoint;
	name_ = os.str();
	LOGD("Created");

	group_ = folder_service.get_group(conn_.hash);

	// Set up timers
	// If there are crashes in these, then wrap the lambdas inside a ScopedAsyncQueue
	ping_timer_.tick_signal.connect([this]{send_ping();});
	timeout_timer_.tick_signal.connect([this]{ws_service_.close(conn_.connection_handle, "Connection timed out");});

	// Start timers
	ping_timer_.start(std::chrono::seconds(20), ScopedTimer::RUN_DEFERRED, ScopedTimer::RESET_TIMER, ScopedTimer::SINGLESHOT);
	bump_timeout();
}

P2PFolder::~P2PFolder() {
	timeout_timer_.stop();
	ping_timer_.stop();

	LOGD("Destroyed");
}

Json::Value P2PFolder::collect_state() {
	Json::Value state;

	std::ostringstream os; os << remote_endpoint();
	state["endpoint"] = os.str();
	state["client_name"] = client_name();
	state["user_agent"] = user_agent();
	state["traffic_stats"] = counter_.heartbeat_json();
	state["rtt"] = Json::Value::UInt64(rtt_.count());

	return state;
}

blob P2PFolder::local_token() {
	return derive_token(folder_group()->secret(), node_key_.public_key());
}

blob P2PFolder::remote_token() {
	return derive_token(folder_group()->secret(), remote_pubkey());
}

void P2PFolder::send_message(const blob& message) {
	counter_.add_up(message.size());
	folder_group()->bandwidth_counter().add_up(message.size());
	ws_service_.send_message(conn_.connection_handle, message);
}

void P2PFolder::perform_handshake() {
	if(!folder_group()) throw protocol_error();

	V1Parser::Handshake message_struct;
	message_struct.auth_token = local_token();
	message_struct.device_name = Config::get()->global_get("client_name").asString();
	message_struct.user_agent = Version::current().user_agent().toStdString();

	send_message(parser_.gen_Handshake(message_struct));
	LOGD("==> HANDSHAKE");
}

/* RPC Actions */
void P2PFolder::choke() {
	if(! am_choking_) {
		send_message(parser_.gen_Choke());
		am_choking_ = true;

		LOGD("==> CHOKE");
	}
}
void P2PFolder::unchoke() {
	if(am_choking_) {
		send_message(parser_.gen_Unchoke());
		am_choking_ = false;

		LOGD("==> UNCHOKE");
	}
}
void P2PFolder::interest() {
	if(! am_interested_) {
		send_message(parser_.gen_Interested());
		am_interested_ = true;

		LOGD("==> INTERESTED");
	}
}
void P2PFolder::uninterest() {
	if(am_interested_) {
		send_message(parser_.gen_NotInterested());
		am_interested_ = false;

		LOGD("==> NOT_INTERESTED");
	}
}

void P2PFolder::post_have_meta(const Meta::PathRevision& revision, const bitfield_type& bitfield) {
	V1Parser::HaveMeta message;
	message.revision = revision;
	message.bitfield = bitfield;
	send_message(parser_.gen_HaveMeta(message));

	LOGD("==> HAVE_META:"
		<< " path_id=" << path_id_readable(message.revision.path_id_)
		<< " revision=" << message.revision.revision_
		<< " bits=" << message.bitfield);
}
void P2PFolder::post_have_chunk(const blob& ct_hash) {
	V1Parser::HaveChunk message;
	message.ct_hash = ct_hash;
	send_message(parser_.gen_HaveChunk(message));

	LOGD("==> HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(ct_hash));
}

void P2PFolder::request_meta(const Meta::PathRevision& revision) {
	V1Parser::MetaRequest message;
	message.revision = revision;
	send_message(parser_.gen_MetaRequest(message));

	LOGD("==> META_REQUEST:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_);
}
void P2PFolder::post_meta(const SignedMeta& smeta, const bitfield_type& bitfield) {
	V1Parser::MetaReply message;
	message.smeta = smeta;
	message.bitfield = bitfield;
	send_message(parser_.gen_MetaReply(message));

	LOGD("==> META_REPLY:"
		<< " path_id=" << path_id_readable(smeta.meta().path_id())
		<< " revision=" << smeta.meta().revision()
		<< " bits=" << bitfield);
}
void P2PFolder::cancel_meta(const Meta::PathRevision& revision) {
	V1Parser::MetaCancel message;
	message.revision = revision;
	send_message(parser_.gen_MetaCancel(message));

	LOGD("==> META_CANCEL:"
		<< " path_id=" << path_id_readable(revision.path_id_)
		<< " revision=" << revision.revision_);
}

void P2PFolder::request_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockRequest message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_.gen_BlockRequest(message));

	LOGD("==> BLOCK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length);
}
void P2PFolder::post_block(const blob& ct_hash, uint32_t offset, const blob& block) {
	V1Parser::BlockReply message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.content = block;
	send_message(parser_.gen_BlockReply(message));

	counter_.add_up_blocks(block.size());
	folder_group()->bandwidth_counter().add_up_blocks(block.size());

	LOGD("==> BLOCK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset);
}
void P2PFolder::cancel_block(const blob& ct_hash, uint32_t offset, uint32_t length) {
	V1Parser::BlockCancel message;
	message.ct_hash = ct_hash;
	message.offset = offset;
	message.length = length;
	send_message(parser_.gen_BlockCancel(message));
	LOGD("==> BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(ct_hash)
		<< " offset=" << offset
		<< " length=" << length);
}

void P2PFolder::handle_message(const blob& message_raw) {
	V1Parser::message_type message_type = parser_.parse_MessageType(message_raw);

	counter_.add_down(message_raw.size());
	folder_group()->bandwidth_counter().add_down(message_raw.size());

	bump_timeout();

	if(ready()) {
		switch(message_type) {
			case V1Parser::CHOKE: handle_Choke(message_raw); break;
			case V1Parser::UNCHOKE: handle_Unchoke(message_raw); break;
			case V1Parser::INTERESTED: handle_Interested(message_raw); break;
			case V1Parser::NOT_INTERESTED: handle_NotInterested(message_raw); break;
			case V1Parser::HAVE_META: handle_HaveMeta(message_raw); break;
			case V1Parser::HAVE_CHUNK: handle_HaveChunk(message_raw); break;
			case V1Parser::META_REQUEST: handle_MetaRequest(message_raw); break;
			case V1Parser::META_REPLY: handle_MetaReply(message_raw); break;
			case V1Parser::META_CANCEL: handle_MetaCancel(message_raw); break;
			case V1Parser::BLOCK_REQUEST: handle_BlockRequest(message_raw); break;
			case V1Parser::BLOCK_REPLY: handle_BlockReply(message_raw); break;
			case V1Parser::BLOCK_CANCEL: handle_BlockCancel(message_raw); break;
			default: throw protocol_error();
		}
	}else{
		handle_Handshake(message_raw);
	}
}

void P2PFolder::handle_Handshake(const blob& message_raw) {
	LOGFUNC();
	auto message_struct = parser_.parse_Handshake(message_raw);
	LOGD("<== HANDSHAKE");

	// Checking authentication using token
	if(message_struct.auth_token != remote_token()) throw auth_error();

	if(conn_.role == WSService::connection::SERVER) perform_handshake();

	client_name_ = message_struct.device_name;
	user_agent_ = message_struct.user_agent;

	LOGD("LV Handshake successful");
	is_handshaken_ = true;

	handshake_performed();
}

void P2PFolder::handle_Choke(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== CHOKE");

	if(! peer_choking_) {
		peer_choking_ = true;
		recv_choke();
	}
}
void P2PFolder::handle_Unchoke(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== UNCHOKE");

	if(peer_choking_) {
		peer_choking_ = false;
		recv_unchoke();
	}
}
void P2PFolder::handle_Interested(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== INTERESTED");

	if(! peer_interested_) {
		peer_interested_ = true;
		recv_interested();
	}
}
void P2PFolder::handle_NotInterested(const blob& message_raw) {
	LOGFUNC();
	LOGD("<== NOT_INTERESTED");

	if(peer_interested_) {
		peer_interested_ = false;
		recv_not_interested();
	}
}

void P2PFolder::handle_HaveMeta(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = parser_.parse_HaveMeta(message_raw);
	LOGD("<== HAVE_META:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_
		<< " bits=" << message_struct.bitfield);

	recv_have_meta(message_struct.revision, message_struct.bitfield);
}
void P2PFolder::handle_HaveChunk(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = parser_.parse_HaveChunk(message_raw);
	LOGD("<== HAVE_BLOCK:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash));
	recv_have_chunk(message_struct.ct_hash);
}

void P2PFolder::handle_MetaRequest(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = parser_.parse_MetaRequest(message_raw);
	LOGD("<== META_REQUEST:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_);

	recv_meta_request(message_struct.revision);
}
void P2PFolder::handle_MetaReply(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = parser_.parse_MetaReply(message_raw, folder_group()->secret());
	LOGD("<== META_REPLY:"
		<< " path_id=" << path_id_readable(message_struct.smeta.meta().path_id())
		<< " revision=" << message_struct.smeta.meta().revision()
		<< " bits=" << message_struct.bitfield);

	recv_meta_reply(message_struct.smeta, message_struct.bitfield);
}
void P2PFolder::handle_MetaCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	LOGFUNC();

	auto message_struct = parser_.parse_MetaCancel(message_raw);
	LOGD("<== META_CANCEL:"
		<< " path_id=" << path_id_readable(message_struct.revision.path_id_)
		<< " revision=" << message_struct.revision.revision_);

	recv_meta_cancel(message_struct.revision);
}

void P2PFolder::handle_BlockRequest(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = parser_.parse_BlockRequest(message_raw);
	LOGD("<== BLOCK_REQUEST:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset);

	recv_block_request(message_struct.ct_hash, message_struct.offset, message_struct.length);
}
void P2PFolder::handle_BlockReply(const blob& message_raw) {
	LOGFUNC();

	auto message_struct = parser_.parse_BlockReply(message_raw);
	LOGD("<== BLOCK_REPLY:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " offset=" << message_struct.offset);

	counter_.add_down_blocks(message_struct.content.size());
	folder_group()->bandwidth_counter().add_down_blocks(message_struct.content.size());

	recv_block_reply(message_struct.ct_hash, message_struct.offset, message_struct.content);
}
void P2PFolder::handle_BlockCancel(const blob& message_raw) {
#   warning "Not implemented yet"
	LOGFUNC();

	auto message_struct = parser_.parse_BlockCancel(message_raw);
	LOGD("<== BLOCK_CANCEL:"
		<< " ct_hash=" << ct_hash_readable(message_struct.ct_hash)
		<< " length=" << message_struct.length
		<< " offset=" << message_struct.offset);

	recv_block_cancel(message_struct.ct_hash, message_struct.offset, message_struct.length);
}

void P2PFolder::bump_timeout() {
	timeout_timer_.start(std::chrono::seconds(120), ScopedTimer::RUN_DEFERRED, ScopedTimer::RESET_TIMER, ScopedTimer::SINGLESHOT);
}

void P2PFolder::send_ping() {
	auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
	ws_service_.ping(conn_.connection_handle, std::to_string(ms_since_epoch.count()));
}

void P2PFolder::handle_ping(std::string payload) {
	bump_timeout();
}

void P2PFolder::handle_pong(std::string payload) {
	bump_timeout();
	try {
		auto now = std::chrono::steady_clock::now().time_since_epoch();
		std::chrono::milliseconds ms_payload(stol(payload));
		if(now - ms_payload > std::chrono::milliseconds(0))
			rtt_ = std::chrono::duration_cast<std::chrono::milliseconds>(now - ms_payload);
	}catch(std::exception& e){}
}

} /* namespace librevault */
