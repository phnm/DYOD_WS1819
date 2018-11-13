#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "../../lib/resolve_type.hpp"
#include "../../lib/storage/base_segment.hpp"
#include "../../lib/storage/dictionary_segment.hpp"
#include "../../lib/storage/value_segment.hpp"

class StorageDictionarySegmentTest : public ::testing::Test {
 protected:
  std::shared_ptr<opossum::ValueSegment<int>> vc_int = std::make_shared<opossum::ValueSegment<int>>();
  std::shared_ptr<opossum::DictionarySegment<int>> dc_int;
  std::shared_ptr<opossum::ValueSegment<std::string>> vc_str = std::make_shared<opossum::ValueSegment<std::string>>();
  std::shared_ptr<opossum::DictionarySegment<std::string>> dc_str;
};

TEST_F(StorageDictionarySegmentTest, CompressSegmentString) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");

  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("string", vc_str);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<std::string>>(col);

  // Test attribute_vector size
  EXPECT_EQ(dict_col->size(), 6u);

  // Test dictionary size (uniqueness)
  EXPECT_EQ(dict_col->unique_values_count(), 4u);

  // Test sorting
  auto dict = dict_col->dictionary();
  EXPECT_EQ((*dict)[0], "Alexander");
  EXPECT_EQ((*dict)[1], "Bill");
  EXPECT_EQ((*dict)[2], "Hasso");
  EXPECT_EQ((*dict)[3], "Steve");
}

TEST_F(StorageDictionarySegmentTest, LowerUpperBound) {
  for (int i = 0; i <= 10; i += 2) vc_int->append(i);
  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  auto dict_col = std::dynamic_pointer_cast<opossum::DictionarySegment<int>>(col);

  EXPECT_EQ(dict_col->lower_bound(4), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->lower_bound(opossum::AllTypeVariant{4}), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(4), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(opossum::AllTypeVariant{4}), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(5), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->lower_bound(opossum::AllTypeVariant{5}), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(5), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(opossum::AllTypeVariant{5}), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(15), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->lower_bound(opossum::AllTypeVariant{15}), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(15), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(opossum::AllTypeVariant{15}), opossum::INVALID_VALUE_ID);
}

TEST_F(StorageDictionarySegmentTest, FailedAppend) {
  for (int i = 0; i <= 10; i += 2) vc_int->append(i);
  auto col = opossum::make_shared_by_data_type<opossum::BaseSegment, opossum::DictionarySegment>("int", vc_int);
  EXPECT_THROW(col->append(opossum::AllTypeVariant{10}), std::runtime_error);
}

TEST_F(StorageDictionarySegmentTest, DataTypeWidth) {
  for (int i = 0; i <= 10; i++) vc_int->append(i);
  dc_int = std::make_shared<opossum::DictionarySegment<int>>(vc_int);
  EXPECT_EQ(dc_int->attribute_vector()->width(), 1);

  for (int i = 0; i <= 350; i++) vc_int->append(i);
  dc_int = std::make_shared<opossum::DictionarySegment<int>>(vc_int);
  EXPECT_EQ(dc_int->attribute_vector()->width(), 2);

  for (int i = 0; i <= 100000; i++) vc_int->append(i);
  dc_int = std::make_shared<opossum::DictionarySegment<int>>(vc_int);
  EXPECT_EQ(dc_int->attribute_vector()->width(), 4);
}

TEST_F(StorageDictionarySegmentTest, ValueRetrieval) {
  for (int i = 0; i < 100; i++) vc_int->append(i);
  for (int i = 0; i < 100; i++) vc_int->append(i);
  for (int i = 0; i < 100; i++) vc_int->append(i);
  dc_int = std::make_shared<opossum::DictionarySegment<int>>(vc_int);

  EXPECT_EQ(dc_int->size(), (size_t)300);
  EXPECT_EQ(dc_int->unique_values_count(), (size_t)100);

  EXPECT_EQ(dc_int->get(0), 0);
  EXPECT_EQ(dc_int->get(57), 57);

  EXPECT_EQ(dc_int->value_by_value_id(opossum::ValueID{23}), 23);
  EXPECT_EQ(dc_int->value_by_value_id(opossum::ValueID{123}), 23);
  EXPECT_EQ(dc_int->value_by_value_id(opossum::ValueID{223}), 23);

  EXPECT_EQ((*dc_int)[23], opossum::AllTypeVariant{23});
  EXPECT_EQ((*dc_int)[123], opossum::AllTypeVariant{23});
  EXPECT_EQ((*dc_int)[223], opossum::AllTypeVariant{23});
}
