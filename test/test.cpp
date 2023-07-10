#include "gtest/gtest.h"
#include "test.pb.h"


TEST(TestBase, TestBase) {
    test::TestBase tb;

    std::string err;
    EXPECT_EQ(tb.ucl_load(std::string("tb.config"), err), 0);

    // check field
    EXPECT_EQ(tb.field_int32(), 123);
    EXPECT_EQ(tb.field_int64(), 456);
    EXPECT_EQ(tb.field_string(), "abc");
    EXPECT_EQ((tb.field_double() - 678.0) < 0.001, true);
    EXPECT_EQ((tb.field_float() - 789.0f) < 0.001f, true);
    EXPECT_EQ(tb.field_enum(), test::TestEnum_Enum_OK);
    EXPECT_EQ(tb.field_bool(), true);
}


TEST(TestRepeat, TestRepeat) {
    test::TestRepeat rtb;

    std::string err;
    EXPECT_EQ(rtb.ucl_load(std::string("rtb.config"), err), 0);

    // check field
    EXPECT_EQ(rtb.field_int32_size()==3
            && rtb.field_int32(0) == 123
            && rtb.field_int32(1) == 234
            && rtb.field_int32(2) == 345, true);
    EXPECT_EQ(rtb.field_int64_size()==3
            && rtb.field_int64(0) == 1123
            && rtb.field_int64(1) == 1234
            && rtb.field_int64(2) == 1345, true);
    EXPECT_EQ(rtb.field_string_size()==3
            && rtb.field_string(0) == "abcd"
            && rtb.field_string(1) == "qwer"
            && rtb.field_string(2) == "rfvt", true);
    EXPECT_EQ(rtb.field_double_size()==3
            && rtb.field_double(0) - 123.1 < 0.001
            && rtb.field_double(1) - 234.2 < 0.001
            && rtb.field_double(2) - 345.3 < 0.001, true);
    EXPECT_EQ(rtb.field_float_size()==3
            && rtb.field_float(0) - 1123.1 < 0.001
            && rtb.field_float(1) - 1234.2 < 0.001
            && rtb.field_float(2) - 1345.3 < 0.001, true);
    EXPECT_EQ(rtb.field_enum_size()==3
            && rtb.field_enum(0) == test::TestEnum_Enum_UNKNOWN
            && rtb.field_enum(1) == test::TestEnum_Enum_OK
            && rtb.field_enum(2) == test::TestEnum_Enum_ERR, true);
    EXPECT_EQ(rtb.field_bool_size()==4
            && rtb.field_bool(0) == true
            && rtb.field_bool(1) == true
            && rtb.field_bool(2) == false
            && rtb.field_bool(3) == false, true);
}

TEST(TestNest, TestNest) {
    test::TestNest nest;

    std::string err;
    EXPECT_EQ(nest.ucl_load(std::string("nest.config"), err), 0);

    // check field
    const auto& tb = nest.tb();
    EXPECT_EQ(tb.field_int32(), 123);
    EXPECT_EQ(tb.field_int64(), 456);
    EXPECT_EQ(tb.field_string(), "abc");
    EXPECT_EQ((tb.field_double() - 678.0) < 0.001, true);
    EXPECT_EQ((tb.field_float() - 789.0f) < 0.001f, true);
    EXPECT_EQ(tb.field_enum(), test::TestEnum_Enum_OK);
    EXPECT_EQ(tb.field_bool(), true);

    // check field
    EXPECT_EQ(nest.rtb_size(), 1);
    const auto& rtb = nest.rtb(0);
    EXPECT_EQ(rtb.field_int32_size()==3
            && rtb.field_int32(0) == 123
            && rtb.field_int32(1) == 234
            && rtb.field_int32(2) == 345, true);
    EXPECT_EQ(rtb.field_int64_size()==3
            && rtb.field_int64(0) == 1123
            && rtb.field_int64(1) == 1234
            && rtb.field_int64(2) == 1345, true);
    EXPECT_EQ(rtb.field_string_size()==3
            && rtb.field_string(0) == "abcd"
            && rtb.field_string(1) == "qwer"
            && rtb.field_string(2) == "rfvt", true);
    EXPECT_EQ(rtb.field_double_size()==3
            && rtb.field_double(0) - 123.1 < 0.001
            && rtb.field_double(1) - 234.2 < 0.001
            && rtb.field_double(2) - 345.3 < 0.001, true);
    EXPECT_EQ(rtb.field_float_size()==3
            && rtb.field_float(0) - 1123.1 < 0.001
            && rtb.field_float(1) - 1234.2 < 0.001
            && rtb.field_float(2) - 1345.3 < 0.001, true);
    EXPECT_EQ(rtb.field_enum_size()==3
            && rtb.field_enum(0) == test::TestEnum_Enum_UNKNOWN
            && rtb.field_enum(1) == test::TestEnum_Enum_OK
            && rtb.field_enum(2) == test::TestEnum_Enum_ERR, true);
    EXPECT_EQ(rtb.field_bool_size()==4
            && rtb.field_bool(0) == true
            && rtb.field_bool(1) == true
            && rtb.field_bool(2) == false
            && rtb.field_bool(3) == false, true);
}

