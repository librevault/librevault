/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
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
#pragma once
#include <util/periodic_process.h>
#include "pch.h"
#include "AbstractStorage.h"

namespace librevault {

class Client;
class Archive : public Loggable {
	friend class ArchiveStrategy;
public:
	Archive(FSFolder& dir, Client& client);
	virtual ~Archive() {}

	void archive(const fs::path& from);

private:
	FSFolder& dir_;
	Client& client_;

	struct ArchiveStrategy {
	public:
		virtual void archive(const fs::path& from) = 0;
		virtual ~ArchiveStrategy(){}

	protected:
		ArchiveStrategy(Archive* parent) : parent_(parent) {}
		Archive* parent_;
	};
	class NoArchive : public ArchiveStrategy {
	public:
		NoArchive(Archive* parent) : ArchiveStrategy(parent) {}
		virtual ~NoArchive(){}
		void archive(const fs::path& from);
	};
	class TrashArchive : public ArchiveStrategy {
	public:
		TrashArchive(Archive* parent);
		virtual ~TrashArchive();
		void archive(const fs::path& from);

	private:
		void maintain_cleanup(PeriodicProcess& process);

		const fs::path archive_path_;
		PeriodicProcess cleanup_process_;
	};
	class TimestampArchive : public ArchiveStrategy {
	public:
		TimestampArchive(Archive* parent);
		virtual ~TimestampArchive(){}
		void archive(const fs::path& from);

	private:
		const fs::path archive_path_;
	};
	std::unique_ptr<ArchiveStrategy> archive_strategy_;

	void move(const fs::path& from, const fs::path& to);
};

} /* namespace librevault */
