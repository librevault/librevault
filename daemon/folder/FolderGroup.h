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
#include "control/FolderParams.h"
#include "p2p/BandwidthCounter.h"
#include "util/blob.h"
#include "util/network.h"
#include <librevault/Secret.h>
#include <librevault/SignedMeta.h>
#include <librevault/util/bitfield_convert.h>
#include <QObject>
#include <QTimer>
#include <QSet>
#include <set>
#include <QHostAddress>

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

class FolderGroup : public QObject {
	Q_OBJECT
	friend class ControlServer;

signals:
	void attached(P2PFolder* remote_ptr);
	void detached(P2PFolder* remote_ptr);

public:
	FolderGroup(FolderParams params, StateCollector* state_collector, io_service& bulk_ios, io_service& serial_ios, QObject* parent);
	virtual ~FolderGroup();

	/* Membership management */
	void attach(P2PFolder* remote_ptr);
	void detach(P2PFolder* remote_ptr);

	bool remotePresent(P2PFolder* folder);

	/* Getters */
	QList<RemoteFolder*> remotes() const;

	inline const FolderParams& params() const {return params_;}

	inline const Secret& secret() const {return params().secret;}
	inline const blob& hash() const {return secret().get_Hash();}
	QByteArray folderid() const {return QByteArray::fromRawData((char*)secret().get_Hash().data(), secret().get_Hash().size());}

	BandwidthCounter& bandwidth_counter() {return bandwidth_counter_;}

	QString log_tag() const;

private:
	const FolderParams params_;
	StateCollector* state_collector_;

	std::unique_ptr<PathNormalizer> path_normalizer_;
	std::unique_ptr<IgnoreList> ignore_list;

	ChunkStorage* chunk_storage_;
	MetaStorage* meta_storage_;

	Uploader* uploader_;
	Downloader* downloader_;
	MetaUploader* meta_uploader_;
	MetaDownloader* meta_downloader_;

	BandwidthCounter bandwidth_counter_;

	QTimer* state_pusher_;

	/* Members */
	QSet<RemoteFolder*> remotes_;

	// Member lookup optimization
	QSet<QByteArray> p2p_folders_digests_;
	QSet<QPair<QHostAddress, quint16>> p2p_folders_endpoints_;

private slots:
	void push_state();
	void handle_indexed_meta(const SignedMeta& smeta);
	void handle_handshake(RemoteFolder* origin);
};

} /* namespace librevault */
