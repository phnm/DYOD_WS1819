#include "value_segment.hpp"

#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "type_cast.hpp"
#include "utils/assert.hpp"
#include "utils/performance_warning.hpp"
#include <boost/variant/get.hpp>

namespace opossum {

template <typename T>
const AllTypeVariant ValueSegment<T>::operator[](const size_t offset) const {
  PerformanceWarning("operator[] used");
  return AllTypeVariant{data.at(offset)};
}

template <typename T>
void ValueSegment<T>::append(const AllTypeVariant& val) {
  // Implementation goes here
  if(val.type() != typeid(T)) {
    throw std::invalid_argument("Added value does not have type of ValueSegment!");
  }
  data.push_back(boost::get<T>(val));
}

template <typename T>
size_t ValueSegment<T>::size() const {
  // Implementation goes here
  return data.size();
}

template <typename T>
const std::vector<T>& ValueSegment<T>::values() const {
  return data;
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(ValueSegment);

}  // namespace opossum
