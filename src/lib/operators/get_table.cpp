#include "get_table.hpp"
#include "../storage/storage_manager.hpp"


namespace opossum {

GetTable::GetTable(const std::string& name) {
    _table = StorageManager::get().get_table(name);
}

std::shared_ptr<const Table> GetTable::_on_execute() {
    return _table;
}

const std::string& GetTable::table_name() const {
    return _table_name;
}

}  // namespace opossum
