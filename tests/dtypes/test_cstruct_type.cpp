//
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "inc_gtest.hpp"

#include <dynd/array.hpp>
#include <dynd/dtypes/cstruct_type.hpp>
#include <dynd/dtypes/struct_type.hpp>
#include <dynd/dtypes/fixedstring_type.hpp>
#include <dynd/dtypes/string_type.hpp>
#include <dynd/dtypes/convert_dtype.hpp>
#include <dynd/dtypes/byteswap_dtype.hpp>

using namespace std;
using namespace dynd;

TEST(CStructDType, Basic) {
    EXPECT_NE(make_cstruct_type(ndt::make_dtype<int>(), "x"),
                    make_cstruct_type(ndt::make_dtype<int>(), "y"));
    EXPECT_NE(make_cstruct_type(ndt::make_dtype<float>(), "x"),
                    make_cstruct_type(ndt::make_dtype<int>(), "x"));
    EXPECT_NE(ndt::type("{x: int32}"), ndt::type("{y: int32}"));
    EXPECT_NE(ndt::type("{x: float32}"), ndt::type("{x: int32}"));
}

struct align_test_struct {
    char f0;  dynd_bool b_;
    char f1;  int8_t i8_;
    char f2;  int16_t i16_;
    char f3;  int32_t i32_;
    char f4;  int64_t i64_;
    char f5;  uint8_t u8_;
    char f6;  uint16_t u16_;
    char f7;  uint32_t u32_;
    char f8;  uint64_t u64_;
    char f9;  float f32_;
    char f10; double f64_;
    char f11; complex<float> cf32_;
    char f12; complex<double> cf64_;
    char f13;
};

TEST(CStructType, Align) {
    ndt::type asdt = ndt::type(
            "{f0:  int8; b_:    bool;     f1:  int8; i8_:  int8;"
            " f2:  int8; i16_:  int16;    f3:  int8; i32_: int32;"
            " f4:  int8; i64_:  int64;    f5:  int8; u8_:  uint8;"
            " f6:  int8; u16_:  uint16;   f7:  int8; u32_: uint32;"
            " f8:  int8; u64_:  uint64;   f9:  int8; f32_: float32;"
            " f10: int8; f64_:  float64;  f11: int8; cf32_: cfloat32;"
            " f12: int8; cf64_: cfloat64; f13: int8}");
    EXPECT_EQ(sizeof(align_test_struct), asdt.get_data_size());
    EXPECT_EQ((size_t)scalar_align_of<align_test_struct>::value, asdt.get_data_alignment());
    const cstruct_type *cd = static_cast<const cstruct_type *>(asdt.extended());
    const size_t *data_offsets = cd->get_data_offsets();
    align_test_struct ats;
#define ATS_OFFSET(field) (reinterpret_cast<size_t>(&ats.field##_) - \
                reinterpret_cast<size_t>(&ats))
    EXPECT_EQ(ATS_OFFSET(b), data_offsets[1]);
    EXPECT_EQ(ATS_OFFSET(i8), data_offsets[3]);
    EXPECT_EQ(ATS_OFFSET(i16), data_offsets[5]);
    EXPECT_EQ(ATS_OFFSET(i32), data_offsets[7]);
    EXPECT_EQ(ATS_OFFSET(i64), data_offsets[9]);
    EXPECT_EQ(ATS_OFFSET(u8), data_offsets[11]);
    EXPECT_EQ(ATS_OFFSET(u16), data_offsets[13]);
    EXPECT_EQ(ATS_OFFSET(u32), data_offsets[15]);
    EXPECT_EQ(ATS_OFFSET(u64), data_offsets[17]);
    EXPECT_EQ(ATS_OFFSET(f32), data_offsets[19]);
    EXPECT_EQ(ATS_OFFSET(f64), data_offsets[21]);
    EXPECT_EQ(ATS_OFFSET(cf32), data_offsets[23]);
    EXPECT_EQ(ATS_OFFSET(cf64), data_offsets[25]);
}

TEST(CStructDType, CreateOneField) {
    ndt::type dt;
    const cstruct_type *tdt;

    // Struct with one field
    dt = make_cstruct_type(ndt::make_dtype<int32_t>(), "x");
    EXPECT_EQ(cstruct_type_id, dt.get_type_id());
    EXPECT_EQ(4u, dt.get_data_size());
    EXPECT_EQ(4u, dt.extended()->get_default_data_size(0, NULL));
    EXPECT_EQ(ndt::make_dtype<int32_t>().get_data_alignment(), dt.get_data_alignment());
    EXPECT_TRUE(dt.is_pod());
    tdt = static_cast<const cstruct_type *>(dt.extended());
    EXPECT_EQ(1u, tdt->get_field_count());
    EXPECT_EQ(ndt::make_dtype<int32_t>(), tdt->get_field_types()[0]);
    EXPECT_EQ(0u, tdt->get_data_offsets_vector()[0]);
    EXPECT_EQ("x", tdt->get_field_names()[0]);
}

struct two_field_struct {
    int64_t a;
    int32_t b;
};

TEST(CStructDType, CreateTwoField) {
    ndt::type dt;
    const cstruct_type *tdt;

    // Struct with two fields
    dt = make_cstruct_type(ndt::make_dtype<int64_t>(), "a", ndt::make_dtype<int32_t>(), "b");
    EXPECT_EQ(cstruct_type_id, dt.get_type_id());
    EXPECT_EQ(sizeof(two_field_struct), dt.get_data_size());
    EXPECT_EQ(sizeof(two_field_struct), dt.extended()->get_default_data_size(0, NULL));
    EXPECT_EQ(ndt::make_dtype<int64_t>().get_data_alignment(), dt.get_data_alignment());
    EXPECT_TRUE(dt.is_pod());
    tdt = static_cast<const cstruct_type *>(dt.extended());
    EXPECT_EQ(2u, tdt->get_field_count());
    EXPECT_EQ(ndt::make_dtype<int64_t>(), tdt->get_field_types()[0]);
    EXPECT_EQ(ndt::make_dtype<int32_t>(), tdt->get_field_types()[1]);
    EXPECT_EQ(0u, tdt->get_data_offsets_vector()[0]);
    EXPECT_EQ(8u, tdt->get_data_offsets_vector()[1]);
    EXPECT_EQ("a", tdt->get_field_names()[0]);
    EXPECT_EQ("b", tdt->get_field_names()[1]);
}

struct three_field_struct {
    int64_t x;
    int32_t y;
    char z[5];
};

TEST(CStructDType, CreateThreeField) {
    ndt::type dt;
    const cstruct_type *tdt;

    // Struct with three fields
    ndt::type d1 = ndt::make_dtype<int64_t>();
    ndt::type d2 = ndt::make_dtype<int32_t>();
    ndt::type d3 = make_fixedstring_type(5, string_encoding_utf_8);
    dt = make_cstruct_type(d1, "x", d2, "y", d3, "z");
    EXPECT_EQ(cstruct_type_id, dt.get_type_id());
    EXPECT_EQ(sizeof(three_field_struct), dt.get_data_size());
    EXPECT_EQ(sizeof(three_field_struct), dt.extended()->get_default_data_size(0, NULL));
    EXPECT_EQ(d1.get_data_alignment(), dt.get_data_alignment());
    EXPECT_TRUE(dt.is_pod());
    tdt = static_cast<const cstruct_type *>(dt.extended());
    EXPECT_EQ(3u, tdt->get_field_count());
    EXPECT_EQ(ndt::make_dtype<int64_t>(), tdt->get_field_types()[0]);
    EXPECT_EQ(ndt::make_dtype<int32_t>(), tdt->get_field_types()[1]);
    EXPECT_EQ(make_fixedstring_type(5, string_encoding_utf_8),
                    tdt->get_field_types()[2]);
    EXPECT_EQ(0u, tdt->get_data_offsets_vector()[0]);
    EXPECT_EQ(8u, tdt->get_data_offsets_vector()[1]);
    EXPECT_EQ(12u, tdt->get_data_offsets_vector()[2]);
    EXPECT_EQ("x", tdt->get_field_names()[0]);
    EXPECT_EQ("y", tdt->get_field_names()[1]);
    EXPECT_EQ("z", tdt->get_field_names()[2]);
}

TEST(CStructDType, ReplaceScalarTypes) {
    ndt::type dt, dt2;

    // Struct with three fields
    ndt::type d1 = ndt::make_dtype<std::complex<double> >();
    ndt::type d2 = ndt::make_dtype<int32_t>();
    ndt::type d3 = make_fixedstring_type(5, string_encoding_utf_8);
    dt = make_cstruct_type(d1, "x", d2, "y", d3, "z");
    dt2 = dt.with_replaced_scalar_types(ndt::make_dtype<int16_t>());
    EXPECT_EQ(make_cstruct_type(
                make_convert_dtype(ndt::make_dtype<int16_t>(), d1), "x",
                make_convert_dtype(ndt::make_dtype<int16_t>(), d2), "y",
                make_convert_dtype(ndt::make_dtype<int16_t>(), d3), "z"),
        dt2);
}

TEST(CStructDType, DTypeAt) {
    ndt::type dt, dt2;

    // Struct with three fields
    ndt::type d1 = ndt::make_dtype<std::complex<double> >();
    ndt::type d2 = ndt::make_dtype<int32_t>();
    ndt::type d3 = make_fixedstring_type(5, string_encoding_utf_8);
    dt = make_cstruct_type(d1, "x", d2, "y", d3, "z");

    // indexing into a dtype with a slice produces a
    // struct dtype (not cstruct dtype) with the subset of fields.
    EXPECT_EQ(make_struct_type(d1, "x", d2, "y"), dt.at(irange() < 2));
    EXPECT_EQ(make_struct_type(d1, "x", d3, "z"), dt.at(irange(0, 3, 2)));
    EXPECT_EQ(make_struct_type(d3, "z", d2, "y"), dt.at(irange(2, 0, -1)));
}

TEST(CStructDType, CanonicalDType) {
    ndt::type dt, dt2;

    // Struct with three fields
    ndt::type d1 = make_convert_dtype<std::complex<double>, float>();
    ndt::type d2 = make_byteswap_dtype<int32_t>();
    ndt::type d3 = make_fixedstring_type(5, string_encoding_utf_32);
    dt = make_cstruct_type(d1, "x", d2, "y", d3, "z");
    EXPECT_EQ(make_cstruct_type(ndt::make_dtype<std::complex<double> >(), "x",
                                ndt::make_dtype<int32_t>(), "y",
                                d3, "z"),
            dt.get_canonical_type());
}

TEST(CStructDType, IsExpression) {
    ndt::type d1 = ndt::make_dtype<float>();
    ndt::type d2 = make_byteswap_dtype<int32_t>();
    ndt::type d3 = make_fixedstring_type(5, string_encoding_utf_32);
    ndt::type d = make_cstruct_type(d1, "x", d2, "y", d3, "z");

    EXPECT_TRUE(d.is_expression());
    EXPECT_FALSE(d.at(irange(0, 3, 2)).is_expression());
}

TEST(CStructDType, PropertyAccess) {
    ndt::type dt = make_cstruct_type(ndt::make_dtype<int>(), "x", ndt::make_dtype<double>(), "y", ndt::make_dtype<short>(), "z");
    nd::array a = nd::empty(dt);
    a(0).vals() = 3;
    a(1).vals() = 4.25;
    a(2).vals() = 5;
    EXPECT_EQ(3, a.p("x").as<int>());
    EXPECT_EQ(4.25, a.p("y").as<double>());
    EXPECT_EQ(5, a.p("z").as<short>());
    EXPECT_THROW(a.p("w"), runtime_error);
}

TEST(CStructDType, EqualDTypeAssign) {
    ndt::type dt = make_cstruct_type(ndt::make_dtype<int>(), "x", ndt::make_dtype<double>(), "y", ndt::make_dtype<short>(), "z");
    nd::array a = nd::make_strided_array(2, dt);
    a(0,0).vals() = 3;
    a(0,1).vals() = 4.25;
    a(0,2).vals() = 5;
    a(1,0).vals() = 6;
    a(1,1).vals() = 7.25;
    a(1,2).vals() = 8;

    nd::array b = nd::make_strided_array(2, dt);
    b.val_assign(a);
    EXPECT_EQ(3,    a(0,0).as<int>());
    EXPECT_EQ(4.25, a(0,1).as<double>());
    EXPECT_EQ(5,    a(0,2).as<short>());
    EXPECT_EQ(6,    a(1,0).as<int>());
    EXPECT_EQ(7.25, a(1,1).as<double>());
    EXPECT_EQ(8,    a(1,2).as<short>());
}

TEST(CStructDType, DifferentDTypeAssign) {
    ndt::type dt = make_cstruct_type(ndt::make_dtype<int>(), "x", ndt::make_dtype<double>(), "y", ndt::make_dtype<short>(), "z");
    nd::array a = nd::make_strided_array(2, dt);
    a(0,0).vals() = 3;
    a(0,1).vals() = 4.25;
    a(0,2).vals() = 5;
    a(1,0).vals() = 6;
    a(1,1).vals() = 7.25;
    a(1,2).vals() = 8;

    ndt::type dt2 = make_cstruct_type(ndt::make_dtype<float>(), "y", ndt::make_dtype<int>(), "z", ndt::make_dtype<uint8_t>(), "x");
    nd::array b = nd::make_strided_array(2, dt2);
    b.val_assign(a);
    EXPECT_EQ(3,    b(0,2).as<int>());
    EXPECT_EQ(4.25, b(0,0).as<double>());
    EXPECT_EQ(5,    b(0,1).as<short>());
    EXPECT_EQ(6,    b(1,2).as<int>());
    EXPECT_EQ(7.25, b(1,0).as<double>());
    EXPECT_EQ(8,    b(1,1).as<short>());
}

TEST(CStructDType, FromStructAssign) {
    ndt::type dt = make_struct_type(ndt::make_dtype<int>(), "x", ndt::make_dtype<double>(), "y", ndt::make_dtype<short>(), "z");
    nd::array a = nd::make_strided_array(2, dt);
    a(0,0).vals() = 3;
    a(0,1).vals() = 4.25;
    a(0,2).vals() = 5;
    a(1,0).vals() = 6;
    a(1,1).vals() = 7.25;
    a(1,2).vals() = 8;

    ndt::type dt2 = make_cstruct_type(ndt::make_dtype<float>(), "y", ndt::make_dtype<int>(), "z", ndt::make_dtype<uint8_t>(), "x");
    nd::array b = nd::make_strided_array(2, dt2);
    b.val_assign(a);
    EXPECT_EQ(3,    b(0,2).as<int>());
    EXPECT_EQ(4.25, b(0,0).as<double>());
    EXPECT_EQ(5,    b(0,1).as<short>());
    EXPECT_EQ(6,    b(1,2).as<int>());
    EXPECT_EQ(7.25, b(1,0).as<double>());
    EXPECT_EQ(8,    b(1,1).as<short>());
}

TEST(CStructDType, SingleCompare) {
    nd::array a, b;
    ndt::type sdt = make_cstruct_type(ndt::make_dtype<int32_t>(), "a",
                    ndt::make_dtype<float>(), "b", ndt::make_dtype<int64_t>(), "c");
    a = nd::empty(sdt);
    b = nd::empty(sdt);

    // Test lexicographic sorting

    // a == b
    a.p("a").vals() = 3;
    a.p("b").vals() = -2.25;
    a.p("c").vals() = 66;
    b.p("a").vals() = 3;
    b.p("b").vals() = -2.25;
    b.p("c").vals() = 66;
    EXPECT_FALSE(a.op_sorting_less(b));
    EXPECT_THROW((a < b), not_comparable_error);
    EXPECT_THROW((a <= b), not_comparable_error);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_THROW((a >= b), not_comparable_error);
    EXPECT_THROW((a > b), not_comparable_error);
    EXPECT_FALSE(b.op_sorting_less(a));
    EXPECT_THROW((b < a), not_comparable_error);
    EXPECT_THROW((b <= a), not_comparable_error);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(b != a);
    EXPECT_THROW((b >= a), not_comparable_error);
    EXPECT_THROW((b > a), not_comparable_error);

    // Different in the first field
    a.p("a").vals() = 3;
    a.p("b").vals() = -2.25;
    a.p("c").vals() = 66;
    b.p("a").vals() = 4;
    b.p("b").vals() = -2.25;
    b.p("c").vals() = 66;
    EXPECT_TRUE(a.op_sorting_less(b));
    EXPECT_THROW((a < b), not_comparable_error);
    EXPECT_THROW((a <= b), not_comparable_error);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_THROW((a >= b), not_comparable_error);
    EXPECT_THROW((a > b), not_comparable_error);
    EXPECT_FALSE(b.op_sorting_less(a));
    EXPECT_THROW((b < a), not_comparable_error);
    EXPECT_THROW((b <= a), not_comparable_error);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_THROW((b >= a), not_comparable_error);
    EXPECT_THROW((b > a), not_comparable_error);

    // Different in the second field
    a.p("a").vals() = 3;
    a.p("b").vals() = -2.25;
    a.p("c").vals() = 66;
    b.p("a").vals() = 3;
    b.p("b").vals() = -2.23;
    b.p("c").vals() = 66;
    EXPECT_TRUE(a.op_sorting_less(b));
    EXPECT_THROW((a < b), not_comparable_error);
    EXPECT_THROW((a <= b), not_comparable_error);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_THROW((a >= b), not_comparable_error);
    EXPECT_THROW((a > b), not_comparable_error);
    EXPECT_FALSE(b.op_sorting_less(a));
    EXPECT_THROW((b < a), not_comparable_error);
    EXPECT_THROW((b <= a), not_comparable_error);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_THROW((b >= a), not_comparable_error);
    EXPECT_THROW((b > a), not_comparable_error);


    // Different in the third field
    a.p("a").vals() = 3;
    a.p("b").vals() = -2.25;
    a.p("c").vals() = 66;
    b.p("a").vals() = 3;
    b.p("b").vals() = -2.25;
    b.p("c").vals() = 1000;
    EXPECT_TRUE(a.op_sorting_less(b));
    EXPECT_THROW((a < b), not_comparable_error);
    EXPECT_THROW((a <= b), not_comparable_error);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_THROW((a >= b), not_comparable_error);
    EXPECT_THROW((a > b), not_comparable_error);
    EXPECT_FALSE(b.op_sorting_less(a));
    EXPECT_THROW((b < a), not_comparable_error);
    EXPECT_THROW((b <= a), not_comparable_error);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_THROW((b >= a), not_comparable_error);
    EXPECT_THROW((b > a), not_comparable_error);
}