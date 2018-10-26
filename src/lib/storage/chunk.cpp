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

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) {
  _segments.push_back(segment);
}

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  // Implementation goes here
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const {
  return _segments.at(column_id);
}

uint16_t Chunk::column_count() const {
  return _segments.size();
}

uint32_t Chunk::size() const {
  auto max_size = 0u;
  for(auto& segment : _segments) {
    max_size = (segment->size() > max_size) ? segment->size() : max_size;
  }
  return max_size;
}

}  // namespace opossum
