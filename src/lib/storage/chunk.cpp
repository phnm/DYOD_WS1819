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

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) { _segments.push_back(segment); }

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == _segments.size(), "New values have different size than number of columns!");
  for (uint16_t i = 0; i < values.size(); i++) {
    _segments.at(i)->append(values.at(i));
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const { return _segments.at(column_id); }

uint16_t Chunk::column_count() const { return ColumnID{_segments.size()}; }

uint32_t Chunk::size() const {
  size_t max_size = 0;
  for (auto& segment : _segments) {
    max_size = std::max(max_size, segment->size());
  }
  return static_cast<uint32_t>(max_size);
}

}  // namespace opossum
