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
#include "pch.h"
#include "AbstractStorage.h"
#include <librevault/Secret.h>
#include "folder/fs/Index.h"
#include "EncStorage.h"
#include "Archive.h"

namespace librevault {

class FSFolder;
class FileAssembler : public Loggable {
public:
	struct error : std::runtime_error {
		error(const std::string& what) : std::runtime_error(what) {}
		error() : error("FileAssembler error") {}
	};

	FileAssembler(FSFolder& dir, ChunkStorage& chunk_storage, Client& client);
	virtual ~FileAssembler() {}

	blob get_chunk_pt(const blob& ct_hash) const;

	// File assembler
	void queue_assemble(const Meta& meta);
	//void disassemble(const std::string& file_path, bool delete_file = true);

private:
	FSFolder& dir_;
	ChunkStorage& chunk_storage_;
	Client& client_;

	Archive archive_;

	const Secret& secret_;
	Index& index_;

	std::set<blob> assemble_queue_;
	std::mutex assemble_queue_mtx_;

	void periodic_assemble_operation(PeriodicProcess& process);
	PeriodicProcess assemble_process_;

	void assemble(const Meta& meta);

	bool assemble_deleted(const Meta& meta);
	bool assemble_symlink(const Meta& meta);
	bool assemble_directory(const Meta& meta);
	bool assemble_file(const Meta& meta);

	void apply_attrib(const Meta& meta);
};

} /* namespace librevault */
