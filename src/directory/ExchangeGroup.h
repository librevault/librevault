/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "../pch.h"

namespace librevault {

class Exchanger;
class AbstractDirectory;
class FSDirectory;
class P2PDirectory;
class ExchangeGroup : public std::enable_shared_from_this<ExchangeGroup> {
public:
	ExchangeGroup(blob hash, Exchanger& exchanger);
	virtual ~ExchangeGroup();

	// FSDirectory actions
	void add(FSDirectory* directory);
	void remove(FSDirectory* directory);
	void announce_revision(const blob& path_hmac, int64_t revision, FSDirectory* directory);

	// P2PDirectory actions
	void add(std::shared_ptr<P2PDirectory> directory);
	void remove(std::shared_ptr<P2PDirectory> directory);
	void announce_revision(const blob& path_hmac, int64_t revision, P2PDirectory* directory);

	const blob& hash() const {return hash_;}

private:
	std::shared_ptr<spdlog::logger> log_;
	Exchanger& exchanger_;

	blob hash_;

	std::string name;	// Basically, printable hash in hex form

	std::set<FSDirectory*> fs_directories_;
	std::set<std::shared_ptr<P2PDirectory>> p2p_directories_;

	void add(AbstractDirectory* directory);
	void remove(AbstractDirectory* directory);
};

} /* namespace librevault */
