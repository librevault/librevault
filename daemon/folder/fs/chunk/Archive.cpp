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
#include <util/file_util.h>
#include <util/regex_escape.h>
#include "Archive.h"

#include "Client.h"
#include "folder/fs/FSFolder.h"
#include "folder/fs/AutoIndexer.h"
#include "ChunkStorage.h"

namespace librevault {

Archive::Archive(FSFolder& dir, Client& client) :
	Loggable(dir, "Archive"),
	dir_(dir),
	client_(client) {

	switch(dir_.params().archive_type) {
		case FolderParams::ArchiveType::NO_ARCHIVE: archive_strategy_ = std::make_unique<NoArchive>(this); break;
		case FolderParams::ArchiveType::TRASH_ARCHIVE: archive_strategy_ = std::make_unique<TrashArchive>(this); break;
		case FolderParams::ArchiveType::TIMESTAMP_ARCHIVE: archive_strategy_ = std::make_unique<TimestampArchive>(this); break;
		//case FolderParams::ArchiveType::BLOCK_ARCHIVE: archive_strategy_ = std::make_unique<NoArchive>(this); break;
		default: throw std::runtime_error("Wrong Archive type");
	}
}

void Archive::archive(const fs::path& from) {
	auto file_type = fs::symlink_status(from).type();

	// Suppress unnecessary events on dir_monitor.
	if(dir_.auto_indexer) dir_.auto_indexer->prepare_deleted_assemble(dir_.normalize_path(from));

	if(file_type == fs::directory_file) {
		if(fs::is_empty(from)) // Okay, just remove this empty directory
			fs::remove(from);
		else {  // Oh, damn, this is very NOT RIGHT! So, we have DELETED directory with NOT DELETED files in it
			for(auto it = fs::directory_iterator(from); it != fs::directory_iterator(); it++)
				archive(it->path()); // TODO: Okay, this is a horrible solution
			fs::remove(from);
		}
	}else if(file_type == fs::regular_file) {
		archive_strategy_->archive(from);
	}else if(file_type == fs::symlink_file || file_type == fs::file_not_found) {
		fs::remove(from);
	}
	// TODO: else
}

void Archive::move(const fs::path& from, const fs::path& to) {
	fs::remove(to);
	try {
		fs::create_directories(to.parent_path());
		fs::rename(from, to);
	}catch(boost::filesystem::filesystem_error& e){
		fs::copy(from, to);
		fs::remove(from);
	}
}

// NoArchive
void Archive::NoArchive::archive(const fs::path& from) {
	fs::remove(from);
}

// TrashArchive
Archive::TrashArchive::TrashArchive(Archive* parent) :
	ArchiveStrategy(parent),
	daily_timer_(parent->client_.ios()),
	archive_path_(parent->dir_.system_path() / "archive") {

	fs::create_directory(archive_path_);
	daily_timer_.expires_from_now(std::chrono::minutes(10));
	daily_timer_.async_wait(std::bind(&Archive::TrashArchive::maintain_cleanup, this, std::placeholders::_1));
}

void Archive::TrashArchive::maintain_cleanup(const boost::system::error_code& ec) {
	parent_->log_->trace() << parent_->log_tag() << BOOST_CURRENT_FUNCTION;
	if(ec == boost::asio::error::operation_aborted) return;

	for(auto it = fs::recursive_directory_iterator(archive_path_); it != fs::recursive_directory_iterator(); it++) {
		time_t time_since_archivation = time(nullptr) - fs::last_write_time(it->path());
		if(time_since_archivation / 60*60*24) {
			fs::remove(it->path());
		}
	}

	daily_timer_.expires_from_now(std::chrono::hours(24));
	daily_timer_.async_wait(std::bind(&Archive::TrashArchive::maintain_cleanup, this, std::placeholders::_1));
}

void Archive::TrashArchive::archive(const fs::path& from) {
	auto archived_path = archive_path_ / fs::path(parent_->dir_.normalize_path(from));
	parent_->log_->trace() << parent_->log_tag() << "Adding an archive item: " << archived_path;
	parent_->move(from, archived_path);
}

// TimestampArchive
Archive::TimestampArchive::TimestampArchive(Archive* parent) :
	ArchiveStrategy(parent),
	archive_path_(parent->dir_.system_path() / "archive") {

	fs::create_directory(archive_path_);
}

void Archive::TimestampArchive::archive(const fs::path& from) {
	// Add a new entry
	auto archived_path = archive_path_ / fs::path(parent_->dir_.normalize_path(from));

	time_t mtime = fs::last_write_time(from);
	std::vector<char> strftime_buf(16);
	strftime(strftime_buf.data(), strftime_buf.size(), "%Y%m%d-%H%M%S", localtime(&mtime));

	std::string suffix = std::string("~")+strftime_buf.data();

	auto timestamped_path = archived_path.stem();
#if BOOST_OS_WINDOWS
	timestamped_path += std::wstring_convert<wchar_t>(suffix);
#else
	timestamped_path += suffix;
#endif
	timestamped_path += archived_path.extension();
	parent_->move(from, timestamped_path);

	// Remove
	std::map<std::string, fs::path> paths;
	std::regex timestamp_regex(regex_escape(from.stem().generic_string()) + R"((~\d{8}-\d{6}))" + regex_escape(from.extension().generic_string()));
	for(auto it = fs::directory_iterator(from.parent_path()); it != fs::directory_iterator(); it++) {
		std::smatch match;
		std::regex_match(it->path().generic_string(), match, timestamp_regex);
		if(!match.empty()) {
			paths.insert({match[1].str(), from});
		}
	}
	if(paths.size() > 5) {
		fs::remove(paths.begin()->second);
	}
}

} /* namespace librevault */
