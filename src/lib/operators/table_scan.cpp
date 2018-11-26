#include "table_scan.hpp"

#include <memory>

#include "resolve_type.hpp"
#include "storage/base_attribute_vector.hpp"
#include "storage/fitted_attribute_vector.hpp"

namespace opossum {

template <typename T>
bool compare(const ScanType& scan_type, const T left, const T right) {
  switch (scan_type) {
    case ScanType::OpEquals:
      return left == right;
    case ScanType::OpNotEquals:
      return left != right;
    case ScanType::OpLessThan:
      return left < right;
    case ScanType::OpLessThanEquals:
      return left <= right;
    case ScanType::OpGreaterThan:
      return left > right;
    case ScanType::OpGreaterThanEquals:
      return left >= right;
    default:
      Fail("Unknown scan operator");
  }
}

TableScan::TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
                     const AllTypeVariant search_value)
    : AbstractOperator(in), _column_id(column_id), _scan_type(scan_type), _search_value(search_value) {}

ColumnID TableScan::column_id() const { return _column_id; }

ScanType TableScan::scan_type() const { return _scan_type; }

const AllTypeVariant& TableScan::search_value() const { return _search_value; }

std::shared_ptr<const Table> TableScan::_on_execute() {
  auto& column_type = _input_table_left()->column_type(_column_id);
  const auto implementation = make_unique_by_data_type<BaseTableScanImpl, TableScanImpl>(column_type);
  return implementation->on_execute(*this);
}

template <typename T>
void TableScan::TableScanImpl<T>::_compare_value_segment(std::shared_ptr<ValueSegment<T>> segment,
                                                         const ScanType& scan_type, const T& search_value,
                                                         std::shared_ptr<PosList> pos_list, ChunkID chunk_id) {
  // retrieve data vector directly since it contains the actual data type (so we don't have to use AllTypeVariant)
  const auto& data = segment->values();
  for (ChunkOffset row_index{0}; row_index < data.size(); row_index++) {
    auto value = data[row_index];
    if (compare(scan_type, value, search_value)) {
      pos_list->emplace_back(RowID{chunk_id, row_index});
    }
  }
}


template <typename T>
void TableScan::TableScanImpl<T>::_compare_dictionary_segment(std::shared_ptr<DictionarySegment<T>> segment,
                                                              const ScanType& scan_type, const T& search_value,
                                                              std::shared_ptr<PosList> pos_list,
                                                              ChunkID chunk_id) {
  auto attribute_vector = segment->attribute_vector();
  auto lower_bound = segment->lower_bound(search_value);
  auto upper_bound = segment->upper_bound(search_value);

  // TODO: append vectors to one another (std::vector::insert) and use std::for_each to transform vectors
  switch (scan_type) {
    case ScanType::OpEquals:
      // if there is no lower bound break since all values are smaller than the search value
      // if the value at the lower bound does not equal the search value, no value equals it
      if (lower_bound == INVALID_VALUE_ID || segment->value_by_value_id(lower_bound) != search_value) {
        break;
      }

      // otherwise check against every value individually
      for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
        auto value_id = attribute_vector->get(row_index);
        if (value_id == lower_bound) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
      }
      break;

    case ScanType::OpNotEquals:
      // if the lower bound equals INVALID_VALUE_ID all values are smaller than the search value
      // if the value of the lower bound does not equal the search value, no value equals it
      if (lower_bound == INVALID_VALUE_ID || segment->value_by_value_id(lower_bound) != search_value) {
        for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
        break;
      }

      // otherwise check against every value individually
      for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
        auto value_id = attribute_vector->get(row_index);
        if (value_id != lower_bound) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
      }
      break;

    case ScanType::OpLessThan:
      // if the lower bound equals INVALID_VALUE_ID all values are smaller than the search value
      if (lower_bound == INVALID_VALUE_ID) {
        for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
        break;
      }

      // otherwise check against every value individually
      for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
        auto value_id = attribute_vector->get(row_index);
        if (value_id < lower_bound) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
      }
      break;

    case ScanType::OpLessThanEquals:
      // if the lower bound equals INVALID_VALUE_ID all values are smaller than the search value
      // if the upper bound equals INVALID_VALUE_ID all values are smaller than or equal to the search value
      if (lower_bound == INVALID_VALUE_ID || upper_bound == INVALID_VALUE_ID) {
        for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
        break;
      }

      // otherwise check against every value individually
      // if there are values that equal the search value, the values can be be compared to the lower bound with <=
      if (segment->value_by_value_id(lower_bound) == search_value) {
        for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
          auto value_id = attribute_vector->get(row_index);
          if (value_id <= lower_bound) {
            pos_list->emplace_back(RowID{chunk_id, row_index});
          }
        }
      } else {
        // otherwise compare to the lower bound with <
        for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
          auto value_id = attribute_vector->get(row_index);
          if (value_id < lower_bound) {
            pos_list->emplace_back(RowID{chunk_id, row_index});
          }
        }
      }
      break;

    case ScanType::OpGreaterThan:
      // if the upper bound is equal to INVALID_VALUE_ID all values are smaller than or equal to the search value
      if (upper_bound == INVALID_VALUE_ID) {
        break;
      }

      // otherwise check against every value individually
      for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
        auto value_id = attribute_vector->get(row_index);
        if (value_id >= upper_bound) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
      }
      break;

    case ScanType::OpGreaterThanEquals:
      // if the lower bound is equal to INVALID_VALUE_ID all values are smaller than the search value
      if (lower_bound == INVALID_VALUE_ID) {
        break;
      }

      // otherwise check against every value individually
      for (ChunkOffset row_index{0}; row_index < attribute_vector->size(); row_index++) {
        auto value_id = attribute_vector->get(row_index);
        if (value_id >= lower_bound) {
          pos_list->emplace_back(RowID{chunk_id, row_index});
        }
      }
      break;
    default:
      Fail("Unknown scan operator");
  }

}

template <typename T>
void TableScan::TableScanImpl<T>::_compare_reference_segment(std::shared_ptr<ReferenceSegment> segment,
                                                             const ScanType& scan_type, const T& search_value,
                                                             std::shared_ptr<PosList> pos_list,
                                                             ChunkID chunk_id, ColumnID column_id) {

  for (const auto& row_id : *(segment->pos_list())) {
    const auto& referenced_chunk = segment->referenced_table()->get_chunk(row_id.chunk_id);
    const auto& referenced_segment = referenced_chunk.get_segment(column_id);

    // the references segment needs to be either a value segment or a dictionary segment
    auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(referenced_segment);
    auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(referenced_segment);

    DebugAssert(value_segment != nullptr || dictionary_segment != nullptr, "Reference segment does not point to value "
                                                                           "or dictionary segment");
    // retrieve the value from the referenced segment
    T value;
    if (value_segment != nullptr) {
      value = value_segment->values()[row_id.chunk_offset];
    } else {
      value = dictionary_segment->get(row_id.chunk_offset);
    }

    if (compare(scan_type, value, search_value)) {
      // use the original row id instead of referencing the reference segment
      pos_list->emplace_back(row_id);
    }
  }
}

template <typename T>
std::shared_ptr<const Table> TableScan::TableScanImpl<T>::on_execute(TableScan& scan_operator) {
  const auto& input_table = scan_operator._input_table_left();

  // these are used if the input is a reference segment
  // in that case we need to correctly reference the input table of that reference segment instead of the input table
  // to this scan
  std::shared_ptr<const Table> referenced_table;
  bool reference_reference_segment = false;

  const auto search_value = type_cast<T>(scan_operator.search_value());
  auto result_row_ids = std::make_shared<PosList>();

  // TODO: check types

  for (ChunkID chunk_id{0}; chunk_id < input_table->chunk_count(); chunk_id++) {
    // retrieve the segment of the searched column from the input table
    const auto& current_chunk = input_table->get_chunk(chunk_id);
    const auto& segment = current_chunk.get_segment(scan_operator.column_id());

    // try to cast to every kind of segment to find out which segment it is
    auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment);
    auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment);
    auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);

    // call the correct compare method for the type of segment we have
    if (value_segment != nullptr) {
      _compare_value_segment(value_segment, scan_operator.scan_type(), search_value, result_row_ids, chunk_id);
    } else if (dictionary_segment != nullptr) {
      _compare_dictionary_segment(dictionary_segment, scan_operator.scan_type(), search_value, result_row_ids,
                                  chunk_id);
    } else if (reference_segment != nullptr) {
      _compare_reference_segment(reference_segment, scan_operator.scan_type(), search_value, result_row_ids, chunk_id, scan_operator.column_id());

      // remember the input table of the reference segment
      reference_reference_segment = true;
      referenced_table = reference_segment->referenced_table();
    } else {
      Fail("Column and search value have differing data types");
    }
  }

  // create the result table
  auto result_table = std::make_shared<Table>();
  for (ColumnID column_id{0}; column_id < input_table->column_count(); column_id++) {
    result_table->add_column_definition(input_table->column_name(column_id), input_table->column_type(column_id));
  }

  // create a reference segment with the same pos_list for each column and add them to a chunk
  auto chunk = std::make_shared<Chunk>();
  auto segment_table = reference_reference_segment ? referenced_table : input_table;
  for (ColumnID column_id{0}; column_id < input_table->column_count(); column_id++) {
    auto reference_segment = std::make_shared<ReferenceSegment>(segment_table, column_id, result_row_ids);
    chunk->add_segment(reference_segment);
  }

  // this replaces the existing chunk since it is empty
  result_table->emplace_chunk(chunk);

  return result_table;
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(TableScan::TableScanImpl);

}  // namespace opossum
