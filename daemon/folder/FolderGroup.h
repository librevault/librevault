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
#include "AbstractFolder.h"
#include "control/FolderParams.h"
#include "p2p/BandwidthCounter.h"
#include "util/network.h"
#include "util/periodic_process.h"

#include <librevault/Secret.h>
#include <librevault/SignedMeta.h>
#include <librevault/util/bitfield_convert.h>

#include <boost/signals2/signal.hpp>
#include <set>
#include <mutex>

namespace librevault {

class RemoteFolder;
class FSFolder;
class P2PFolder;

class PathNormalizer;
class IgnoreList;
class StateCollector;

class ChunkStorage;
class MetaStorage;

class MetaUploader;
class MetaDownloader;
class Uploader;
class Downloader;

class FolderGroup {
	friend class ControlServer;
public:
	struct error : std::runtime_error {
		error(const char* what) : std::runtime_error(what) {}
		error() : error("FolderGroup error") {}
	};

	struct attach_error : error {
		attach_error() : error("Could not attach remote to FolderGroup") {}
	};

	FolderGroup(FolderParams params, StateCollector& state_collector, io_service& bulk_ios, io_service& serial_ios);
	virtual ~FolderGroup();

	/* Membership management */
	void attach(std::shared_ptr<P2PFolder> remote_ptr);
	void detach(std::shared_ptr<P2PFolder> remote_ptr);

	boost::signals2::signal<void(std::shared_ptr<P2PFolder>)> attached_signal;
	boost::signals2::signal<void(std::shared_ptr<P2PFolder>)> detached_signal;

	bool have_p2p_dir(const tcp_endpoint& endpoint);
	bool have_p2p_dir(const blob& pubkey);

	/* Getters */
	std::set<std::shared_ptr<RemoteFolder>> remotes() const;
	inline std::set<std::shared_ptr<P2PFolder>> p2p_dirs() const {return p2p_folders_;}

	inline const FolderParams& params() const {return params_;}

	inline const Secret& secret() const {return params().secret;}
	inline const blob& hash() const {return secret().get_Hash();}

	BandwidthCounter& bandwidth_counter() {return bandwidth_counter_;}

	std::string log_tag() const;
private:
	const FolderParams params_;
	StateCollector& state_collector_;
	io_service& serial_ios_;

	std::unique_ptr<PathNormalizer> path_normalizer_;
	std::unique_ptr<IgnoreList> ignore_list;

	std::unique_ptr<ChunkStorage> chunk_storage;
	std::unique_ptr<MetaStorage> meta_storage_;

	std::unique_ptr<Uploader> uploader_;
	std::unique_ptr<Downloader> downloader_;
	std::unique_ptr<MetaUploader> meta_uploader_;
	std::unique_ptr<MetaDownloader> meta_downloader_;

	BandwidthCounter bandwidth_counter_;

	std::unique_ptr<PeriodicProcess> state_pusher_;

	/* Members */
	mutable std::mutex p2p_folders_mtx_;

	std::set<std::shared_ptr<P2PFolder>> p2p_folders_;

	// Member lookup optimization
	std::set<blob> p2p_folders_pubkeys_;
	std::set<tcp_endpoint> p2p_folders_endpoints_;

	void handle_indexed_meta(const SignedMeta& smeta);

	void handle_handshake(std::shared_ptr<RemoteFolder> origin);
};

} /* namespace librevault */
