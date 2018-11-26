#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "dictionary_segment.hpp"
#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"
#include "value_segment.hpp"

namespace opossum {

Table::Table(const uint32_t chunk_size) : _chunk_size{chunk_size} {
  _chunks.emplace_back(std::make_shared<Chunk>());
  _chunk_compression_status.emplace_back(false);
}

void Table::add_column_definition(const std::string& name, const std::string& type) {
  // Implementation goes here
}

void Table::add_column(const std::string& name, const std::string& type) {
  Assert(_column_ids_by_name.count(name) == 0, "Column with that name already exists!");
  auto new_column_id = ColumnID{static_cast<uint16_t>(_column_names.size())};
  _column_names.emplace_back(name);
  _column_types.emplace_back(type);
  _column_ids_by_name[name] = new_column_id;

  for (auto& chunk : _chunks) {
    auto new_segment = make_shared_by_data_type<BaseSegment, ValueSegment>(type);
    chunk->add_segment(new_segment);
  }
}

void Table::append(std::vector<AllTypeVariant> values) {
  auto& last_chunk = _chunks.back();
  if (last_chunk->size() < _chunk_size) {
    last_chunk->append(values);
  } else {
    auto new_chunk = std::make_shared<Chunk>();

    for (auto& column_type : _column_types) {
      auto segment = make_shared_by_data_type<BaseSegment, ValueSegment>(column_type);
      new_chunk->add_segment(segment);
    }
    new_chunk->append(values);
    _chunks.emplace_back(new_chunk);
    _chunk_compression_status.emplace_back(false);
  }
}

uint16_t Table::column_count() const { return static_cast<uint16_t>(_column_names.size()); }

void Table::create_new_chunk() {
  // Implementation goes here
}

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

void Table::compress_chunk(ChunkID chunk_id) {
  Assert(chunk_id < _chunks.size() - 1, "Only immutable chunks can be compressed (last chunk ist mutable).");
  {
    auto guard = std::lock_guard(_chunk_compression_mutex);
    if (_chunk_compression_status[chunk_id]) {
      return;
    }
    _chunk_compression_status[chunk_id] = true;
  }
  Chunk& chunk_to_compress = get_chunk(chunk_id);
  auto compressed_chunk = std::make_shared<Chunk>();
  auto chunk_columns = chunk_to_compress.column_count();
  for (ColumnID column_id{0}; column_id < chunk_columns; column_id++) {
    auto segment = chunk_to_compress.get_segment(column_id);
    auto type_string = column_type(column_id);
    auto dict_segment = make_shared_by_data_type<BaseSegment, DictionarySegment>(type_string, segment);
    compressed_chunk->add_segment(dict_segment);
  }
  _chunks[chunk_id] = compressed_chunk;
}

void emplace_chunk(Chunk chunk) {
  // Implementation goes here
}

}  // namespace opossum
