#pragma once

#include "base_attribute_vector.hpp"
#include "utils/assert.hpp"

namespace opossum {
class FittedAttributeVector : public BaseAttributeVector {
  public:
  /**
   * Creates a Dictionary segment from a given value segment.
   */
    explicit FittedAttributeVector(const size_t segment_size, const ValueID& invalid_id) {
      _dictionary_references = std::vector<ValueID>(segment_size, invalid_id);
    }

    // returns the value id at a given position
    ValueID get(const size_t i) const {
      return _dictionary_references.at(i);
    }

    // sets the value id at a given position
    void set(const size_t i, const ValueID value_id) {
      _dictionary_references.at(i) = value_id;
    };

    // returns the number of values
    size_t size() const {
      return _dictionary_references.size();
    }

    // returns the width of biggest value id in bytes
    AttributeVectorWidth width() const {
        // TODO
        return 8;
    }

  protected:
    std::vector<ValueID > _dictionary_references;

};

}  // namespace opossum
