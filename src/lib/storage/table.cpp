#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "value_segment.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

void Table::add_column(const std::string& name, const std::string& type) {
  DebugAssert(_column_ids_by_name.count(name) == 0, "Column with that name already exists!");
  auto new_column_id = ChunkID{_column_names.size()};
  _column_names.push_back(name);
  _column_types.push_back(type);
  _column_ids_by_name[name] = new_column_id;

  for (auto& chunk : _chunks) {
    auto new_segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
    chunk->add_segment(new_segment);
  }
}

void Table::append(std::vector<AllTypeVariant> values) {
  auto last_chunk = _chunks.back();
  if (last_chunk->size() < _chunk_size) {
    last_chunk->append(values);
  } else {
    auto new_chunk = std::make_shared<Chunk>();

    for (auto& column_type : _column_types) {
      auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(column_type);
      new_chunk->add_segment(segment);
    }
    new_chunk->append(values);
    _chunks.push_back(new_chunk);
  }
}

uint16_t Table::column_count() const { return static_cast<uint16_t>(_column_names.size()); }

uint64_t Table::row_count() const {
  uint64_t num_rows = 0;
  for (auto& chunk : _chunks) {
    num_rows += chunk->size();
  }
  return num_rows;
}

ChunkID Table::chunk_count() const { return ChunkID{static_cast<uint32_t>(_chunks.size())}; }

ColumnID Table::column_id_by_name(const std::string& column_name) const { return _column_ids_by_name.at(column_name); }

uint32_t Table::chunk_size() const { return _chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(ColumnID column_id) const { return _column_names.at(column_id); }

const std::string& Table::column_type(ColumnID column_id) const { return _column_types.at(column_id); }

Chunk& Table::get_chunk(ChunkID chunk_id) { return *_chunks.at(chunk_id); }

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return *_chunks.at(chunk_id); }

}  // namespace opossum
