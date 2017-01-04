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
#include "FolderGroup.h"

#include "IgnoreList.h"
#include "PathNormalizer.h"
#include "control/StateCollector.h"
#include "folder/chunk/ChunkStorage.h"
#include "folder/meta/Index.h"
#include "folder/meta/MetaStorage.h"
#include "folder/transfer/MetaDownloader.h"
#include "folder/transfer/MetaUploader.h"
#include "folder/transfer/Uploader.h"
#include "folder/transfer/Downloader.h"
#include "p2p/P2PFolder.h"

namespace librevault {

FolderGroup::FolderGroup(FolderParams params, StateCollector& state_collector, io_service& bulk_ios, io_service& serial_ios, QObject* parent) :
		QObject(parent),
		params_(std::move(params)),
		state_collector_(state_collector),
		serial_ios_(serial_ios) {
	LOGFUNC();

	/* Creating directories */
	bool path_created = fs::create_directories(params_.path);
	bool system_path_created = fs::create_directories(params_.system_path);
#if BOOST_OS_WINDOWS
	SetFileAttributes(params_.system_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

	LOGD("New folder:"
		<< " Key type=" << (char)params_.secret.get_type()
		<< " Path" << (path_created ? " created" : "") << "=" << params_.path
		<< " System path" << (system_path_created ? " created" : "") << "=" << params_.system_path);

	state_collector_.folder_state_set(params_.secret.get_Hash(), "secret", params_.secret.string());

	/* Initializing components */
	path_normalizer_ = std::make_unique<PathNormalizer>(params_);
	ignore_list = std::make_unique<IgnoreList>(params_, *path_normalizer_);

	meta_storage_ = std::make_unique<MetaStorage>(params_, *ignore_list, *path_normalizer_, state_collector_, bulk_ios);
	chunk_storage = std::make_unique<ChunkStorage>(params_, *meta_storage_, *path_normalizer_, bulk_ios, serial_ios);

	uploader_ = std::make_unique<Uploader>(*chunk_storage);
	downloader_ = std::make_unique<Downloader>(params_, *meta_storage_, *chunk_storage, serial_ios);
	meta_uploader_ = std::make_unique<MetaUploader>(*meta_storage_, *chunk_storage);
	meta_downloader_ = std::make_unique<MetaDownloader>(*meta_storage_, *downloader_);

	state_pusher_ = new QTimer(this);

	// Connecting signals and slots
	connect(meta_storage_->index.get(), &Index::metaAdded, this, &FolderGroup::handle_indexed_meta);
	connect(chunk_storage.get(), &ChunkStorage::chunkAdded, this, [this](const blob& ct_hash){
		downloader_->notify_local_chunk(ct_hash);
		uploader_->broadcast_chunk(remotes(), ct_hash);
	});
	connect(state_pusher_, &QTimer::timeout, this, &FolderGroup::push_state);

	// Set up state pusher
	state_pusher_->setInterval(1000);
	state_pusher_->start();

	// Go through index
	serial_ios_.dispatch([=]{
		for(auto& smeta : meta_storage_->index->get_meta())
			handle_indexed_meta(smeta);
	});
}

FolderGroup::~FolderGroup() {
	state_pusher_->stop();

	state_collector_.folder_state_purge(params_.secret.get_Hash());
	LOGFUNC();
}

/* Actions */
void FolderGroup::handle_indexed_meta(const SignedMeta& smeta) {
	Meta::PathRevision revision = smeta.meta().path_revision();
	bitfield_type bitfield = chunk_storage->make_bitfield(smeta.meta());

	downloader_->notify_local_meta(smeta, bitfield);
	meta_uploader_->broadcast_meta(remotes(), revision, bitfield);
}

// RemoteFolder actions
void FolderGroup::handle_handshake(std::shared_ptr<RemoteFolder> origin) {
	origin->recv_choke.connect([origin = std::weak_ptr<RemoteFolder>(origin), this]{
		serial_ios_.post([=]{downloader_->handle_choke(origin.lock());});
	});
	origin->recv_unchoke.connect([origin = std::weak_ptr<RemoteFolder>(origin), this]{
		serial_ios_.post([=]{downloader_->handle_unchoke(origin.lock());});
	});
	origin->recv_interested.connect([origin = std::weak_ptr<RemoteFolder>(origin), this]{
		serial_ios_.post([=]{uploader_->handle_interested(origin.lock());});
	});
	origin->recv_not_interested.connect([origin = std::weak_ptr<RemoteFolder>(origin), this]{
		serial_ios_.post([=]{uploader_->handle_not_interested(origin.lock());});
	});

	origin->recv_have_meta.connect([origin = std::weak_ptr<RemoteFolder>(origin), this](const Meta::PathRevision& revision, const bitfield_type& bitfield){
		serial_ios_.post([=]{meta_downloader_->handle_have_meta(origin.lock(), revision, bitfield);});
	});
	origin->recv_have_chunk.connect([origin = std::weak_ptr<RemoteFolder>(origin), this](const blob& ct_hash){
		serial_ios_.post([=]{downloader_->notify_remote_chunk(origin.lock(), ct_hash);});
	});

	origin->recv_meta_request.connect([origin = std::weak_ptr<RemoteFolder>(origin), this](Meta::PathRevision path_revision){
		serial_ios_.post([=]{meta_uploader_->handle_meta_request(origin.lock(), path_revision);});
	});
	origin->recv_meta_reply.connect([origin = std::weak_ptr<RemoteFolder>(origin), this](const SignedMeta& smeta, const bitfield_type& bitfield){
		serial_ios_.post([=]{meta_downloader_->handle_meta_reply(origin.lock(), smeta, bitfield);});
	});

	origin->recv_block_request.connect([origin = std::weak_ptr<RemoteFolder>(origin), this](const blob& ct_hash, uint32_t offset, uint32_t size){
		serial_ios_.post([=]{uploader_->handle_block_request(origin.lock(), ct_hash, offset, size);});
	});
	origin->recv_block_reply.connect([origin = std::weak_ptr<RemoteFolder>(origin), this](const blob& ct_hash, uint32_t offset, const blob& block){
		serial_ios_.post([=]{downloader_->put_block(ct_hash, offset, block, origin.lock());});
	});

	serial_ios_.post([origin, this]{meta_uploader_->handle_handshake(origin);});
}

void FolderGroup::attach(std::shared_ptr<P2PFolder> remote_ptr) {
	if(have_p2p_dir(remote_ptr->remote_endpoint()) || have_p2p_dir(remote_ptr->digest())) throw attach_error();

	p2p_folders_.insert(remote_ptr);
	p2p_folders_endpoints_.insert(remote_ptr->remote_endpoint());
	p2p_folders_digests_.insert(remote_ptr->digest());

	LOGD("Attached remote " << remote_ptr->displayName().toStdString());

	remote_ptr->handshake_performed.connect([remote_ptr = std::weak_ptr<RemoteFolder>(remote_ptr), this]{handle_handshake(remote_ptr.lock());});

	emit attached(remote_ptr);
}

void FolderGroup::detach(std::shared_ptr<P2PFolder> remote_ptr) {
	downloader_->erase_remote(remote_ptr);

	p2p_folders_digests_.erase(remote_ptr->digest());
	p2p_folders_endpoints_.erase(remote_ptr->remote_endpoint());
	p2p_folders_.erase(remote_ptr);

	LOGD("Detached remote " << remote_ptr->displayName().toStdString());

	emit detached(remote_ptr);
}

bool FolderGroup::have_p2p_dir(const tcp_endpoint& endpoint) {
	return p2p_folders_endpoints_.find(endpoint) != p2p_folders_endpoints_.end();
}

bool FolderGroup::have_p2p_dir(const QByteArray& digest) {
	return p2p_folders_digests_.find(digest) != p2p_folders_digests_.end();
}

std::set<std::shared_ptr<RemoteFolder>> FolderGroup::remotes() const {
	return std::set<std::shared_ptr<RemoteFolder>>(p2p_folders_.begin(), p2p_folders_.end());
}

std::string FolderGroup::log_tag() const {
	return std::string("[") + (!params_.path.empty() ? params_.path : params_.system_path).string() + "] ";
}

void FolderGroup::push_state() {
	// peers
	Json::Value peers_array;
	for(auto& p2p_folder : p2p_folders_) {
		peers_array.append(p2p_folder->collect_state());
	}
	state_collector_.folder_state_set(params_.secret.get_Hash(), "peers", peers_array);
	// bandwidth
	state_collector_.folder_state_set(params_.secret.get_Hash(), "traffic_stats", bandwidth_counter_.heartbeat_json());
}

} /* namespace librevault */
