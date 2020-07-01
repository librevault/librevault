#pragma once
#include <cstdint>
#include <map>

namespace librevault {

template <typename OffsetT = uint64_t>
class AvailabilityMap {
 public:
  struct error : public std::runtime_error {
    error() : std::runtime_error("AvailabilityMap allocation error") {}
  };

  using offset_type = OffsetT;
  using underlying_container = std::map<offset_type, offset_type>;
  using block_type = std::pair<offset_type, offset_type>;
  using const_iterator = typename underlying_container::const_iterator;

  AvailabilityMap(offset_type size) : size_original_(size), size_left_(size) {
    available_map_.insert({0, size_original_});
  }

  static bool slice_superset(block_type subset, block_type superset, block_type& block_left, block_type& block_right) {
    // Initializing with 'invalid' values
    block_left = block_type(0, 0);
    block_right = block_type(0, 0);

    if (subset.second == 0 || superset.second == 0) return false;

    auto subset_last_byte_offset = subset.first + subset.second - 1;
    auto superset_last_byte_offset = superset.first + superset.second - 1;

    if (subset.first < superset.first || subset_last_byte_offset > superset_last_byte_offset) return false;

    if (subset.first != superset.first) {
      block_left.first = superset.first;
      block_left.second = subset.first - superset.first;
    }

    if (subset_last_byte_offset != superset_last_byte_offset) {
      block_right.first = subset_last_byte_offset + 1;
      block_right.second = superset_last_byte_offset - subset_last_byte_offset;
    }
    return true;
  }

  std::pair<const_iterator, bool> insert(block_type block) {
    if (block.first >= size_original_ || block.first + block.second > size_original_ || available_map_.empty())
      return {end(), false};

    auto space_it = available_map_.upper_bound(block.first);  // This returns the block, following the inserted.
    if (space_it == available_map_.begin()) return {end(), false};
    --space_it;

    block_type block_left, block_right;
    if (slice_superset(block, *space_it, block_left, block_right)) {
      available_map_.erase(space_it);
      size_left_ -= block.second;

      const_iterator right_it;

      if (block_left != block_type(0, 0)) available_map_.insert(block_left);
      if (block_right != block_type(0, 0))
        right_it = available_map_.insert(block_right).first;
      else
        right_it = available_map_.upper_bound(block.first);

      return {right_it, true};
    } else
      return {end(), false};
  };

  const_iterator begin() { return available_map_.cbegin(); }
  const_iterator end() { return available_map_.cend(); }

  offset_type size_left() const { return size_left_; }
  offset_type size_original() const { return size_original_; }

  bool full() const { return size_left() == 0; }
  bool empty() const { return size_left() == size_original(); }

 private:
  offset_type size_original_, size_left_;
  underlying_container available_map_;
};

}  // namespace librevault
