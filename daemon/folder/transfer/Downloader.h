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
#include "folder/RemoteFolder.h"
#include "util/AvailabilityMap.h"
#include "util/blob.h"
#include "util/file_util.h"
#include "util/log.h"
#include <QCache>
#include <QFile>
#include <QTimer>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <chrono>
#include <unordered_map>

#define CLUSTERED_COEFFICIENT 10.0f
#define IMMEDIATE_COEFFICIENT 20.0f
#define RARITY_COEFFICIENT 25.0f

namespace librevault {

class FolderParams;
class MetaStorage;
class ChunkStorage;

/* GlobalFilePool is a singleton class, used to open/close files automatically to reduce simultaneously open file descriptors */
class GlobalFilePool {
public:
	static GlobalFilePool* get_instance() {
		static GlobalFilePool* instance;
		if(!instance)
			instance = new GlobalFilePool();
		return instance;
	}

	QFile* getFile(QString path, bool release = false);

private:
	QCache<QString, QFile> opened_files_;
};

/* MissingChunk constructs a chunk in a file. If complete(), then an encrypted chunk is located in  */
struct MissingChunk {
	MissingChunk(QString system_path, blob ct_hash, uint32_t size);

	// File-related accessors
	QFile* release_chunk();

	// Content-related accessors
	void put_block(uint32_t offset, const blob& content);

	// Size-related functions
	uint64_t size() const {return file_map_.size_original();}
	bool complete() const {return file_map_.full();}

	// AvailabilityMap accessors
	const AvailabilityMap<uint32_t>& file_map() const {return file_map_;}

	/* Request-oriented functions */
	struct BlockRequest {
		uint32_t offset;
		uint32_t size;
		std::chrono::steady_clock::time_point started;
	};
	std::unordered_multimap<RemoteFolder*, BlockRequest> requests;
	QHash<RemoteFolder*, std::shared_ptr<RemoteFolder::InterestGuard>> owned_by;

	const blob ct_hash_;

private:
	AvailabilityMap<uint32_t> file_map_;
	QString chunk_location_;
};

class WeightedDownloadQueue {
	struct Weight {
		bool clustered = false;
		bool immediate = false;

		size_t owned_by = 0;
		size_t remotes_count = 0;

		float value() const;
		bool operator<(const Weight& b) const {return value() > b.value();}
		bool operator==(const Weight& b) const {return value() == b.value();}
		bool operator!=(const Weight& b) const {return !(*this == b);}
	};
	using weight_ordered_chunks_t = boost::bimap<
		boost::bimaps::unordered_set_of<std::shared_ptr<MissingChunk>>,
		boost::bimaps::multiset_of<Weight>
	>;
	using queue_left_value = weight_ordered_chunks_t::left_value_type;
	using queue_right_value = weight_ordered_chunks_t::right_value_type;
	weight_ordered_chunks_t weight_ordered_chunks_;

	Weight get_current_weight(std::shared_ptr<MissingChunk> chunk);
	void reweight_chunk(std::shared_ptr<MissingChunk> chunk, Weight new_weight);

public:
	void add_chunk(std::shared_ptr<MissingChunk> chunk);
	void remove_chunk(std::shared_ptr<MissingChunk> chunk);

	void set_overall_remotes_count(size_t count);
	void set_chunk_remotes_count(std::shared_ptr<MissingChunk> chunk, size_t count);

	void mark_clustered(std::shared_ptr<MissingChunk> chunk);
	void mark_immediate(std::shared_ptr<MissingChunk> chunk);

	std::list<std::shared_ptr<MissingChunk>> chunks() const;
};

class Downloader : public QObject {
	Q_OBJECT
	LOG_SCOPE("Downloader");
signals:
	void chunkDownloaded(blob ct_hash, QFile* chunk_f);

public:
	Downloader(const FolderParams& params, MetaStorage* meta_storage, QObject* parent);
	~Downloader();

public slots:
	void notify_local_meta(const SignedMeta& smeta, const bitfield_type& bitfield);
	void notify_local_chunk(const blob& ct_hash, bool mark_clustered = true);

	void notify_remote_meta(RemoteFolder* remote, const Meta::PathRevision& revision, bitfield_type bitfield);
	void notify_remote_chunk(RemoteFolder* remote, const blob& ct_hash);

	void handle_choke(RemoteFolder* remote);
	void handle_unchoke(RemoteFolder* remote);

	void put_block(const blob& ct_hash, uint32_t offset, const blob& data, RemoteFolder* from);

	void trackRemote(RemoteFolder* remote);
	void untrackRemote(RemoteFolder* remote);

private:
	const FolderParams& params_;
	MetaStorage* meta_storage_;

	std::map<blob, std::shared_ptr<MissingChunk>> missing_chunks_;
	WeightedDownloadQueue download_queue_;

	size_t requests_overall() const;

	/* Request process */
	QTimer* maintain_timer_;

	void maintain_requests();
	bool request_one();
	RemoteFolder* find_node_for_request(std::shared_ptr<MissingChunk> chunk);

	/* Node management */
	QSet<RemoteFolder*> remotes_;
};

} /* namespace librevault */
