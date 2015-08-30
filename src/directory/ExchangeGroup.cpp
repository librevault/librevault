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
#include "ExchangeGroup.h"
#include "Exchanger.h"
#include "Abstract.h"
#include "../../contrib/crypto/Hex.h"
#include "../../contrib/crypto/Base32.h"

namespace librevault {

ExchangeGroup::ExchangeGroup(blob hash, Exchanger& exchanger) :
		log_(spdlog::get("Librevault")), exchanger_(exchanger), hash_(std::move(hash)),
		name(crypto::Hex().to(crypto::BinaryArray(hash_))) {
	exchanger_.add(this);
	log_->debug() << "Created exchange group " << name;
}

ExchangeGroup::~ExchangeGroup() {
	log_->debug() << "Removed exchange group " << name;
	exchanger_.remove(this);
}

void ExchangeGroup::add(FSDirectory* directory) {
	fs_directories_.insert(directory);
	add((AbstractDirectory*)directory);
}

void ExchangeGroup::add(std::shared_ptr<P2PDirectory> directory) {
	p2p_directories_.insert(directory);
	add((AbstractDirectory*)directory.get());
}

void ExchangeGroup::add(AbstractDirectory* directory) {
	log_->debug() << "Added " << directory->name() << " to exchange group " << name;
}

void ExchangeGroup::remove(FSDirectory* directory) {
	remove((AbstractDirectory*)directory);
	fs_directories_.erase(directory);
}

void ExchangeGroup::remove(std::shared_ptr<P2PDirectory> directory) {
	remove((AbstractDirectory*)directory.get());
	p2p_directories_.erase(directory);
}

void ExchangeGroup::remove(AbstractDirectory* directory) {
	log_->debug() << "Removed " << directory->name() << " from exchange group " << name;
}

void ExchangeGroup::announce_revision(const blob& path_hmac, int64_t revision, P2PDirectory* directory) {
	log_->debug() << "Announced revision " << revision << " of " << (std::string)crypto::Base32().to(path_hmac) << " from " << directory->name();
}

void ExchangeGroup::announce_revision(const blob& path_hmac, int64_t revision, FSDirectory* directory) {
	log_->debug() << "Announced revision " << revision << " of " << (std::string)crypto::Base32().to(path_hmac) << " from " << directory->name();
}

} /* namespace librevault */
