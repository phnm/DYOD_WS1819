#pragma once

#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base_segment.hpp"
#include "chunk.hpp"

#include "type_cast.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

class TableStatistics;

// A table is partitioned horizontally into a number of chunks
class Table : private Noncopyable {
 public:
  // creates a table
  // the parameter specifies the maximum chunk size, i.e., partition size
  // default is the maximum chunk size minus 1. A table holds always at least one chunk
  explicit Table(const uint32_t chunk_size = std::numeric_limits<ChunkOffset>::max() - 1) {
    _chunk_size = chunk_size;
    _chunks.push_back(std::make_shared<Chunk>());
  };

  // we need to explicitly set the move constructor to default when
  // we overwrite the copy constructor
  Table(Table&&) = default;
  Table& operator=(Table&&) = default;

  // returns the number of columns (cannot exceed ColumnID (uint16_t))
  uint16_t column_count() const;

  // Returns the number of rows.
  // This number includes invalidated (deleted) rows.
  // Use approx_valid_row_count() for an approximate count of valid rows instead.
  uint64_t row_count() const;

  // returns the number of chunks (cannot exceed ChunkID (uint32_t))
  ChunkID chunk_count() const;

  // returns the chunk with the given id
  Chunk& get_chunk(ChunkID chunk_id);
  const Chunk& get_chunk(ChunkID chunk_id) const;

  // Adds a chunk to the table. If the first chunk is empty, it is replaced.
  void emplace_chunk(Chunk chunk);

  // Returns a list of all column names.
  const std::vector<std::string>& column_names() const;

  // returns the column name of the nth column
  const std::string& column_name(ColumnID column_id) const;

  // returns the column type of the nth column
  const std::string& column_type(ColumnID column_id) const;

  // Returns the column with the given name.
  // This method is intended for debugging purposes only.
  // It does not verify whether a column name is unambiguous.
  ColumnID column_id_by_name(const std::string& column_name) const;

  // return the maximum chunk size (cannot exceed ChunkOffset (uint32_t))
  uint32_t chunk_size() const;

  // adds a column to the end, i.e., right, of the table
  // the added column should have the same length as existing columns (if any)
  void add_column(const std::string& name, const std::string& type);

  // inserts a row at the end of the table
  // note this is slow and not thread-safe and should be used for testing purposes only
  void append(std::vector<AllTypeVariant> values);

 protected:
  uint32_t _chunk_size;
  std::vector<std::shared_ptr<Chunk>> _chunks;
  std::vector<std::string> _column_names;
  std::vector<std::string> _column_types;
  std::map<std::string, ColumnID> _column_ids_by_name;
};
}  // namespace opossum
