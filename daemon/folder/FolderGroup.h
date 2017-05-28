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
#include "util/BandwidthCounter.h"
#include "Secret.h"
#include "SignedMeta.h"
#include "conv_bitfield.h"
#include <QObject>
#include <QTimer>
#include <QSet>
#include <set>
#include <QHostAddress>
#include <QLoggingCategory>

namespace librevault {

Q_DECLARE_LOGGING_CATEGORY(log_folder);

class Peer;

class IgnoreList;
class StateCollector;

class ChunkStorage;
class MetaStorage;

class MetaUploader;
class MetaDownloader;
class Uploader;
class Downloader;

class PeerPool;

class FolderGroup : public QObject {
	Q_OBJECT

public:
	FolderGroup(FolderParams params, PeerPool* pool, StateCollector* state_collector, QObject* parent);
	virtual ~FolderGroup();

	/* Getters */
	inline const FolderParams& params() const {return params_;}

private:
	const FolderParams params_;
	PeerPool* pool_;
	StateCollector* state_collector_;

	std::unique_ptr<IgnoreList> ignore_list;

	ChunkStorage* chunk_storage_;
	MetaStorage* meta_storage_;

	Uploader* uploader_;
	Downloader* downloader_;
	MetaUploader* meta_uploader_;
	MetaDownloader* meta_downloader_;

	QTimer* state_pusher_;

private slots:
	void push_state();
	void handle_indexed_meta(const SignedMeta& smeta);
	void handleNewPeer(Peer* peer);
};

} /* namespace librevault */
