#pragma once

#include <memory>
#include <string>
#include <vector>

#include "abstract_operator.hpp"

namespace opossum {

// operator to retrieve a table from the StorageManager by specifying its name
class GetTable : public AbstractOperator {
 public:
  explicit GetTable(const std::string name);

  const std::string table_name() const;

 protected:
  std::shared_ptr<const Table> _on_execute() override;

  // Using a string reference causes the variable to become corrupt. Therefore we copy the value here.
  const std::string _table_name;
};
}  // namespace opossum
