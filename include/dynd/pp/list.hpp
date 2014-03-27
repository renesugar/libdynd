//
// Copyright (C) 2011-14 Irwin Zaid, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__LIST_HPP_
#define _DYND__LIST_HPP_

#include <dynd/pp/comparision.hpp>
#include <dynd/pp/gen.hpp>
#include <dynd/pp/if.hpp>
#include <dynd/pp/logical.hpp>

#define DYND_PP_EVAL DYND_PP_MSC_EVAL
#define DYND_PP_DBL_EVAL(A) DYND_PP_EVAL(DYND_PP_EVAL(A))

#define DYND_PP_IS_NULL(...) DYND_PP__IS_NULL(DYND_PP_HAS_COMMA(__VA_ARGS__), \
    DYND_PP_HAS_COMMA(DYND_PP_TO_COMMA __VA_ARGS__), DYND_PP_HAS_COMMA(__VA_ARGS__ ()), \
    DYND_PP_HAS_COMMA(DYND_PP_TO_COMMA __VA_ARGS__ ()))
#define DYND_PP__IS_NULL(A, B, C, D) DYND_PP_HAS_COMMA(DYND_PP_CAT_8(DYND_PP__IS_EMPTY_, \
    A, _, B, _, C, _, D))
#define DYND_PP__IS_EMPTY_0_0_0_0 DYND_PP__IS_EMPTY_0_0_0_0
#define DYND_PP__IS_EMPTY_0_0_0_1 ,
#define DYND_PP__IS_EMPTY_0_0_1_0 DYND_PP__IS_EMPTY_0_0_1_0
#define DYND_PP__IS_EMPTY_0_0_1_1 DYND_PP__IS_EMPTY_0_0_1_1
#define DYND_PP__IS_EMPTY_0_1_0_0 DYND_PP__IS_EMPTY_0_1_0_0
#define DYND_PP__IS_EMPTY_0_1_0_1 DYND_PP__IS_EMPTY_0_1_0_1
#define DYND_PP__IS_EMPTY_0_1_1_0 DYND_PP__IS_EMPTY_0_1_1_0
#define DYND_PP__IS_EMPTY_0_1_1_1 DYND_PP__IS_EMPTY_0_1_1_1
#define DYND_PP__IS_EMPTY_1_0_0_0 DYND_PP__IS_EMPTY_1_0_0_0
#define DYND_PP__IS_EMPTY_1_0_0_1 DYND_PP__IS_EMPTY_1_0_0_1
#define DYND_PP__IS_EMPTY_1_0_1_0 DYND_PP__IS_EMPTY_1_0_1_0
#define DYND_PP__IS_EMPTY_1_0_1_1 DYND_PP__IS_EMPTY_1_0_1_1
#define DYND_PP__IS_EMPTY_1_1_0_0 DYND_PP__IS_EMPTY_1_1_0_0
#define DYND_PP__IS_EMPTY_1_1_0_1 DYND_PP__IS_EMPTY_1_1_0_1
#define DYND_PP__IS_EMPTY_1_1_1_0 DYND_PP__IS_EMPTY_1_1_1_0
#define DYND_PP__IS_EMPTY_1_1_1_1 DYND_PP__IS_EMPTY_1_1_1_1

#define DYND_PP_IS_EMPTY(A) DYND_PP_IS_NULL(DYND_PP_ID A)

#define DYND_PP_MERGE(A, B) DYND_PP_IF_ELSE(DYND_PP_LEN(A))(DYND_PP_IF_ELSE(DYND_PP_LEN(B))((DYND_PP_ID A, \
    DYND_PP_ID B))(A))(DYND_PP_IF_ELSE(DYND_PP_LEN(B))(B)(()))

#define DYND_PP_PREPEND(ITEM, A) DYND_PP_MERGE((ITEM), A)
#define DYND_PP_APPEND(ITEM, A) DYND_PP_MERGE(A, (ITEM))

#define DYND_PP_GET(INDEX, A) DYND_PP_FIRST(DYND_PP_SLICE_FROM(INDEX, A))

#define DYND_PP_SET(INDEX, VALUE, A) DYND_PP_MERGE(DYND_PP_APPEND(VALUE, DYND_PP_SLICE_TO(INDEX, A)), \
    DYND_PP_SLICE_FROM(DYND_PP_INC(INDEX), A))

#define DYND_PP_DEL(INDEX, A) DYND_PP_MERGE(DYND_PP_SLICE_TO(INDEX, A), \
    DYND_PP_SLICE_FROM(DYND_PP_INC(INDEX), A))

#define DYND_PP_FIRST DYND_PP_HEAD

#define DYND_PP_LAST(A) DYND_PP_GET(DYND_PP_DEC(DYND_PP_LEN(A)), A)

#define DYND_PP_SLICE(START, STOP, STEP, A) DYND_PP_SLICE_WITH(STEP, \
    DYND_PP_SLICE_TO(DYND_PP_SUB(STOP, START), DYND_PP_SLICE_FROM(START, A)))

#define DYND_PP_RANGE(...) DYND_PP_EVAL(DYND_PP_CAT_2(DYND_PP_RANGE_, DYND_PP_LEN((__VA_ARGS__)))(__VA_ARGS__))
#define DYND_PP_RANGE_1(STOP) DYND_PP_RANGE_2(0, STOP)
#define DYND_PP_RANGE_2(START, STOP) DYND_PP_RANGE_3(START, STOP, 1)
#define DYND_PP_RANGE_3(START, STOP, STEP) DYND_PP_SLICE(START, STOP, STEP, DYND_PP_INTS)

#define DYND_PP_ZEROS(COUNT) DYND_PP_REPEAT(0, COUNT)
#define DYND_PP_ONES(COUNT) DYND_PP_REPEAT(1, COUNT)

#define DYND_PP_ALL(A) DYND_PP_IF_ELSE(DYND_PP_EQ(DYND_PP_LEN(A), 1))( \
    DYND_PP_BOOL(DYND_PP_HEAD(A)))(DYND_PP_REDUCE(DYND_PP_AND, A))

#define DYND_PP_ANY(A) DYND_PP_IF_ELSE(DYND_PP_EQ(DYND_PP_LEN(A), 1))( \
    DYND_PP_BOOL(DYND_PP_FIRST(A)))(DYND_PP_REDUCE(DYND_PP_OR, A))

#endif
