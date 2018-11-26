#include "storage_manager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cstdio>

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager instance;
  return instance;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  Assert(_tables_by_name.find(name) == _tables_by_name.end(), "Table with that name already exists!");
  _tables_by_name.emplace(std::make_pair(name, table));
}

void StorageManager::drop_table(const std::string& name) {
  DebugAssert(_tables_by_name.find(name) != _tables_by_name.end(), "Table does not exist!");
  _tables_by_name.erase(name);
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const {
  Assert(_tables_by_name.find(name) != _tables_by_name.end(), "Table does not exist!");
  return _tables_by_name.find(name)->second;
}

bool StorageManager::has_table(const std::string& name) const { return _tables_by_name.count(name) == 1; }

std::vector<std::string> StorageManager::table_names() const {
  auto table_names = std::vector<std::string>();
  for (const auto& kv_pair : _tables_by_name) {
    table_names.emplace_back(kv_pair.first);
  }
  return table_names;
}

void StorageManager::print(std::ostream& out) const {
  for (const auto& [name, table] : _tables_by_name) {
    out << "(" << name << ", " << table->column_count() << ", " << table->row_count() << ", ";
    out << table->chunk_count() << ")" << std::endl;
  }
}

void StorageManager::reset() { _tables_by_name.clear(); }

}  // namespace opossum
