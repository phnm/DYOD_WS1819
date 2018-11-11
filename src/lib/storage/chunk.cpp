#include <algorithm>
#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base_segment.hpp"
#include "chunk.hpp"

#include "utils/assert.hpp"

namespace opossum {

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) { _segments.emplace_back(segment); }

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  Assert(values.size() == _segments.size(), "New values have different size than number of columns!");
  for (uint16_t column_index = 0; column_index < values.size(); column_index++) {
    _segments.at(column_index)->append(values.at(column_index));
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const { return _segments.at(column_id); }

uint16_t Chunk::column_count() const { return ColumnID{static_cast<uint16_t>(_segments.size())}; }

uint32_t Chunk::size() const {
  size_t max_size = 0;
  for (auto& segment : _segments) {
    max_size = std::max(max_size, segment->size());
  }
  return static_cast<uint32_t>(max_size);
}

}  // namespace opossum
