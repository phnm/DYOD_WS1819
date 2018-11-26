#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/table.hpp"
#include "storage/value_segment.hpp"
#include "type_cast.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

class TableScan : public AbstractOperator {
 public:
  TableScan(const std::shared_ptr<const AbstractOperator> in, ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value);

  ~TableScan() = default;

  ColumnID column_id() const;
  ScanType scan_type() const;
  const AllTypeVariant& search_value() const;

 protected:
  std::shared_ptr<const Table> _on_execute() override;
  const ColumnID _column_id;
  const ScanType _scan_type;
  const AllTypeVariant _search_value;

  class BaseTableScanImpl {
   public:
    virtual ~BaseTableScanImpl() = default;
    virtual std::shared_ptr<const Table> on_execute(TableScan& scan_operator) = 0;
  };

  template <typename T>
  class TableScanImpl : public BaseTableScanImpl {
   public:
    std::shared_ptr<const Table> on_execute(TableScan& scan_operator) override;

   protected:
    void _compare_value_segment(std::shared_ptr<ValueSegment<T>> segment, const ScanType& scan_type,
                                const T& search_value, std::shared_ptr<PosList> pos_list, ChunkID chunk_id);
    void _compare_dictionary_segment(std::shared_ptr<DictionarySegment<T>> segment, const ScanType& scan_type,
                                     const T& search_value, std::shared_ptr<PosList> pos_list, ChunkID chunk_id);
    void _compare_reference_segment(std::shared_ptr<ReferenceSegment> segment, const ScanType& scan_type,
                                    const T& search_value, std::shared_ptr<PosList> pos_list, ChunkID chunk_id);
  };
};

}  // namespace opossum
