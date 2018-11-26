#include <limits>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "storage/fitted_attribute_vector.hpp"

class FittedAttributeVectorTest : public ::testing::Test {
 protected:
  std::shared_ptr<opossum::FittedAttributeVector<uint8_t>> uint8_vector;
  std::shared_ptr<opossum::FittedAttributeVector<uint16_t>> uint16_vector;
  std::shared_ptr<opossum::FittedAttributeVector<uint32_t>> uint32_vector;
};

TEST_F(FittedAttributeVectorTest, DataType) {
  uint8_vector = std::make_shared<opossum::FittedAttributeVector<uint8_t>>(10, 11);
  EXPECT_EQ(uint8_vector->width(), 1);
  EXPECT_EQ(uint8_vector->get(0), opossum::ValueID{11});

  uint16_vector =
      std::make_shared<opossum::FittedAttributeVector<uint16_t>>(1234, std::numeric_limits<uint16_t>::max());
  EXPECT_EQ(uint16_vector->width(), 2);
  EXPECT_EQ(uint16_vector->get(0), opossum::ValueID{std::numeric_limits<uint16_t>::max()});
  EXPECT_EQ(uint16_vector->get(0), opossum::ValueID{static_cast<uint16_t>(std::numeric_limits<uint32_t>::max())});

  uint32_vector = std::make_shared<opossum::FittedAttributeVector<uint32_t>>(10, 11);
  EXPECT_EQ(uint32_vector->width(), 4);
  EXPECT_EQ(uint32_vector->get(2), opossum::ValueID{11});
}

TEST_F(FittedAttributeVectorTest, Size) {
  uint8_vector = std::make_shared<opossum::FittedAttributeVector<uint8_t>>(10, 11);
  EXPECT_EQ(uint8_vector->size(), (size_t)10);
}

TEST_F(FittedAttributeVectorTest, InsertAndRead) {
  uint8_vector = std::make_shared<opossum::FittedAttributeVector<uint8_t>>(10, 11);
  uint8_vector->set(0, opossum::ValueID{6});
  uint8_vector->set(3, opossum::ValueID{4});
  EXPECT_THROW(uint8_vector->set(13, opossum::ValueID{5}), std::out_of_range);

  EXPECT_EQ(uint8_vector->get(0), opossum::ValueID{6});
  EXPECT_EQ(uint8_vector->get(3), opossum::ValueID{4});

  if (IS_DEBUG) {
    EXPECT_THROW(uint8_vector->set(4, opossum::ValueID{12}), std::logic_error);
  }
}
