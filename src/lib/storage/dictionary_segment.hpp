#pragma once

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "all_type_variant.hpp"
#include "base_attribute_vector.hpp"
#include "base_segment.hpp"
#include "fitted_attribute_vector.hpp"
#include "type_cast.hpp"
#include "types.hpp"

namespace opossum {

// Even though ValueIDs do not have to use the full width of ValueID (uint32_t), this will also work for smaller ValueID
// types (uint8_t, uint16_t) since after a down-cast INVALID_VALUE_ID will look like their numeric_limit::max()
constexpr ValueID INVALID_VALUE_ID{std::numeric_limits<ValueID::base_type>::max()};

// Dictionary is a specific segment type that stores all its values in a vector
template <typename T>
class DictionarySegment : public BaseSegment {
 public:
  /**
   * Creates a Dictionary segment from a given value segment.
   */
  explicit DictionarySegment(const std::shared_ptr<BaseSegment>& base_segment) {
    build_dictionary(base_segment);
    auto num_values = base_segment->size();
    initialize_attribute_vector(num_values);

    for (uint32_t row_index = 0; row_index < num_values; row_index++) {
      auto value = type_cast<T>((*base_segment)[row_index]);
      auto value_id = lower_bound(value);
      _attribute_vector->set(row_index, value_id);
    }
  }

  // SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
  // the DictionarySegment in this file. Replace the method signatures with actual implementations.

  // return the value at a certain position. If you want to write efficient operators, back off!
  const AllTypeVariant operator[](const size_t i) const override {
    return AllTypeVariant{get(i)};
  }

  // return the value at a certain position.
  const T get(const size_t i) const { return (*_dictionary)[_attribute_vector->get(i)]; }

  // dictionary segments are immutable
  void append(const AllTypeVariant&) override {
    throw std::runtime_error("Dictionary segments are immutable. Can't append value");
  }

  // returns an underlying dictionary
  std::shared_ptr<const std::vector<T>> dictionary() const { return _dictionary; }

  // returns an underlying data structure
  std::shared_ptr<const BaseAttributeVector> attribute_vector() const { return _attribute_vector; }

  // return the value represented by a given ValueID
  const T& value_by_value_id(ValueID value_id) const { return _dictionary->at(value_id); }

  // returns the first value ID that refers to a value >= the search value
  // returns INVALID_VALUE_ID if all values are smaller than the search value
  ValueID lower_bound(T value) const {
    auto value_iterator = std::lower_bound(_dictionary->begin(), _dictionary->end(), value);
    if (value_iterator == _dictionary->end()) {
      return INVALID_VALUE_ID;
    } else {
      return ValueID{static_cast<uint32_t>(value_iterator - _dictionary->begin())};
    }
  }

  // same as lower_bound(T), but accepts an AllTypeVariant
  ValueID lower_bound(const AllTypeVariant& value) const { return lower_bound(type_cast<T>(value)); }

  // returns the first value ID that refers to a value > the search value
  // returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
  ValueID upper_bound(T value) const {
    auto value_iterator = std::upper_bound(_dictionary->begin(), _dictionary->end(), value);
    if (value_iterator == _dictionary->end()) {
      return INVALID_VALUE_ID;
    } else {
      return ValueID{static_cast<uint32_t>(value_iterator - _dictionary->begin())};
    }
  }

  // same as upper_bound(T), but accepts an AllTypeVariant
  ValueID upper_bound(const AllTypeVariant& value) const { return upper_bound(type_cast<T>(value)); }

  // return the number of unique_values (dictionary entries)
  size_t unique_values_count() const { return _dictionary->size(); }

  // return the number of entries
  size_t size() const override { return _attribute_vector->size(); }

 protected:
  // stores the unique values of the value segment
  std::shared_ptr<std::vector<T>> _dictionary;
  // stores for every row of the value segment the reference to the value (index in dictionary)
  std::shared_ptr<BaseAttributeVector> _attribute_vector;

  void build_dictionary(const std::shared_ptr<BaseSegment>& base_segment) {
    _dictionary = std::make_shared<std::vector<T>>();

    // copy values from base segment element wise (since we don't have access to the whole collection)
    for (uint32_t row_index = 0; row_index < base_segment->size(); row_index++) {
      _dictionary->emplace_back(type_cast<T>((*base_segment)[row_index]));
    }

    // use std::set to remove duplicates and somehow sort the values
    auto unique_values = std::set<T>(_dictionary->begin(), _dictionary->end());
    _dictionary->assign(unique_values.begin(), unique_values.end());
  }

  void initialize_attribute_vector(const size_t segment_size) {
    auto num_distinct_entries = _dictionary->size();
    DebugAssert(num_distinct_entries < static_cast<size_t>(INVALID_VALUE_ID),
                "Dictionary too large to be represented by ValueIDs.");
    if (num_distinct_entries < static_cast<uint8_t>(INVALID_VALUE_ID)) {
      _attribute_vector =
          std::make_shared<FittedAttributeVector<uint8_t>>(segment_size, static_cast<uint8_t>(INVALID_VALUE_ID));
    } else if (num_distinct_entries < static_cast<uint16_t>(INVALID_VALUE_ID)) {
      _attribute_vector =
          std::make_shared<FittedAttributeVector<uint16_t>>(segment_size, static_cast<uint16_t>(INVALID_VALUE_ID));
    } else {
      _attribute_vector =
          std::make_shared<FittedAttributeVector<uint32_t>>(segment_size, static_cast<uint32_t>(INVALID_VALUE_ID));
    }
  }
};

}  // namespace opossum
