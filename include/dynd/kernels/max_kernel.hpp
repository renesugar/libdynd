//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/kernels/base_kernel.hpp>
#include <dynd/kernels/base_strided_kernel.hpp>
#include <dynd/types/callable_type.hpp>

namespace dynd {
namespace nd {

  template <type_id_t Src0TypeID>
  struct max_kernel : base_strided_kernel<max_kernel<Src0TypeID>, 1> {
    typedef typename type_of<Src0TypeID>::type src0_type;
    typedef src0_type dst_type;

    void single(char *dst, char *const *src)
    {
      if (*reinterpret_cast<src0_type *>(src[0]) > *reinterpret_cast<dst_type *>(dst)) {
        *reinterpret_cast<dst_type *>(dst) = *reinterpret_cast<src0_type *>(src[0]);
      }
    }

    void strided(char *dst, intptr_t dst_stride, char *const *src, const intptr_t *src_stride, size_t count)
    {

      char *src0 = src[0];
      intptr_t src0_stride = src_stride[0];
      for (size_t i = 0; i < count; ++i) {
        if (*reinterpret_cast<src0_type *>(src0) > *reinterpret_cast<dst_type *>(dst)) {
          *reinterpret_cast<dst_type *>(dst) = *reinterpret_cast<src0_type *>(src0);
        }
        dst += dst_stride;
        src0 += src0_stride;
      }
    }
  };

  template <>
  struct max_kernel<complex_float32_id> : base_strided_kernel<max_kernel<complex_float32_id>, 1> {
    typedef complex<float> src0_type;
    typedef src0_type dst_type;

    void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src))
    {
      throw std::runtime_error("nd::max is not implemented for complex types");
    }
  };

  template <>
  struct max_kernel<complex_float64_id> : base_strided_kernel<max_kernel<complex_float64_id>, 1> {
    typedef complex<double> src0_type;
    typedef src0_type dst_type;

    void single(char *DYND_UNUSED(dst), char *const *DYND_UNUSED(src))
    {
      throw std::runtime_error("nd::max is not implemented for complex types");
    }
  };

} // namespace dynd::nd
} // namespace dynd
