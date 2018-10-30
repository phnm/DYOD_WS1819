#include "storage_manager.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager instance;
  return instance;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  DebugAssert(_tables_by_name.count(name) == 0, "Table with that name already exists!");
  _tables_by_name[name] = table;
}

void StorageManager::drop_table(const std::string& name) {
  DebugAssert(_tables_by_name.count(name) == 1, "Table does not exist!");
  _tables_by_name.erase(name);
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const { return _tables_by_name.at(name); }

bool StorageManager::has_table(const std::string& name) const { return _tables_by_name.count(name) == 1; }

std::vector<std::string> StorageManager::table_names() const {
  auto table_names = std::vector<std::string>();
  for (auto& kv_pair : _tables_by_name) {
    table_names.push_back(kv_pair.first);
  }
  return table_names;
}

void StorageManager::print(std::ostream& out) const {
  for (auto& kv_pair : _tables_by_name) {
    auto name = kv_pair.first;
    auto table = kv_pair.second;
    out << "(" << name << ", " << table->column_count() << ", " << table->row_count() << ", ";
    out << table->chunk_count() << ")" << std::endl;
  }
}

void StorageManager::reset() { _tables_by_name.clear(); }

}  // namespace opossum
