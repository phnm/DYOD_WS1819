#pragma once

#include <limits>
#include <vector>

#include "base_attribute_vector.hpp"
#include "utils/assert.hpp"

namespace opossum {
template <typename T>
class FittedAttributeVector : public BaseAttributeVector {
 public:
  /**
   * Creates a Dictionary segment from a given value segment.
   */
  explicit FittedAttributeVector(const size_t segment_size, const T& invalid_id) : _invalid_id(invalid_id) {
    _dictionary_references = std::vector<T>(segment_size, _invalid_id);
  }

  // returns the value id at a given position
  ValueID get(const size_t i) const { return ValueID{_dictionary_references.at(i)}; }

  // sets the value id at a given position
  void set(const size_t i, const ValueID value_id) {
    DebugAssert(static_cast<T>(value_id) < _invalid_id, "ValueID is too large for type of Attribute Segment");
    _dictionary_references.at(i) = static_cast<T>(value_id);
  }

  // returns the number of values
  size_t size() const { return _dictionary_references.size(); }

  // returns the width of biggest value id in bytes
  AttributeVectorWidth width() const { return AttributeVectorWidth{sizeof(T)}; }

 protected:
  std::vector<T> _dictionary_references;
  const T _invalid_id;
};

}  // namespace opossum
