#include "table_scan.hpp"

#include <memory>

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
      throw std::logic_error("Unknown scan operator");
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
                                                         const ScanType& scan_type, const T search_value,
                                                         std::shared_ptr<PosList> pos_list, const ChunkID chunk_id) {
  for (ChunkOffset row_index{0}; row_index < segment->size(); row_index++) {
    // TODO: use data vector directly
    auto value = type_cast<T>(segment->values()[row_index]);
    if (compare(scan_type, value, search_value)) {
      pos_list->emplace_back(RowID{chunk_id, row_index});
    }
  }
}

template <typename T>
void TableScan::TableScanImpl<T>::_compare_dictionary_segment(std::shared_ptr<DictionarySegment<T>> segment,
                                                              const ScanType& scan_type, const T search_value,
                                                              std::shared_ptr<PosList> pos_list,
                                                              const ChunkID chunk_id) {
  for (ChunkOffset row_index{0}; row_index < segment->size(); row_index++) {
    // TODO: don't uncompress the whole segment
    auto value = segment->get(row_index);
    if (compare(scan_type, value, search_value)) {
      pos_list->emplace_back(RowID{chunk_id, row_index});
    }
  }
}

template <typename T>
void TableScan::TableScanImpl<T>::_compare_reference_segment(std::shared_ptr<ReferenceSegment> segment,
                                                             const ScanType& scan_type, const T search_value,
                                                             std::shared_ptr<PosList> pos_list,
                                                             const ChunkID chunk_id) {
  for (ChunkOffset row_index{0}; row_index < segment->size(); row_index++) {
    // TODO: there is a faster way using pos_list
    auto value = type_cast<T>((*segment)[row_index]);
    if (compare(scan_type, value, search_value)) {
      pos_list->emplace_back(RowID{chunk_id, row_index});
    }
  }
}

template <typename T>
std::shared_ptr<const Table> TableScan::TableScanImpl<T>::on_execute(TableScan& scan_operator) {
  const auto& input_table = scan_operator._input_table_left();
  std::shared_ptr<const Table> referenced_table;
  bool reference_reference_segment = false;

  const auto search_value = type_cast<T>(scan_operator.search_value());
  auto result_row_ids = std::make_shared<PosList>();

  // TODO: check types

  for (ChunkID chunk_id{0}; chunk_id < input_table->chunk_count(); chunk_id++) {
    const auto& current_chunk = input_table->get_chunk(chunk_id);
    const auto& segment = current_chunk.get_segment(scan_operator.column_id());

    auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(segment);
    auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(segment);
    auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);

    if (value_segment != nullptr) {
      _compare_value_segment(value_segment, scan_operator.scan_type(), search_value, result_row_ids, chunk_id);
    } else if (dictionary_segment != nullptr) {
      _compare_dictionary_segment(dictionary_segment, scan_operator.scan_type(), search_value, result_row_ids,
                                  chunk_id);
    } else if (reference_segment != nullptr) {
      _compare_reference_segment(reference_segment, scan_operator.scan_type(), search_value, result_row_ids, chunk_id);
      reference_reference_segment = true;
      referenced_table = reference_segment->referenced_table();
    } else {
      throw std::logic_error("Column and search value have differing data types");
    }
  }

  auto result_table = std::make_shared<Table>();
  for (ColumnID column_id{0}; column_id < input_table->column_count(); column_id++) {
    result_table->add_column_definition(input_table->column_name(column_id), input_table->column_type(column_id));
  }

  auto chunk = std::make_shared<Chunk>();
  auto segment_table = reference_reference_segment ? referenced_table : input_table;
  for (ColumnID column_id{0}; column_id < input_table->column_count(); column_id++) {
    auto reference_segment = std::make_shared<ReferenceSegment>(segment_table, column_id, result_row_ids);
    chunk->add_segment(reference_segment);
  }
  result_table->emplace_chunk(chunk)

      return result_table;
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(TableScan::TableScanImpl);

}  // namespace opossum
