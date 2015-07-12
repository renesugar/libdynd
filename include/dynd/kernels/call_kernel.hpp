#pragma once

#include <dynd/kernels/base_virtual_kernel.hpp>

namespace dynd {
namespace nd {

  template <typename CallableType>
  struct call_kernel : base_virtual_kernel<call_kernel<CallableType>> {
    static void data_init(char *DYND_UNUSED(static_data), size_t data_size,
                          char *data, const ndt::type &dst_tp, intptr_t nsrc,
                          const ndt::type *src_tp, const nd::array &kwds,
                          const std::map<dynd::nd::string, ndt::type> &tp_vars)
    {
      CallableType::get()->data_init(CallableType::get()->static_data,
                                     data_size, data, dst_tp, nsrc, src_tp,
                                     kwds, tp_vars);
    }

    static void
    resolve_dst_type(char *DYND_UNUSED(static_data), size_t data_size,
                     char *data, ndt::type &dst_tp, intptr_t nsrc,
                     const ndt::type *src_tp, const dynd::nd::array &kwds,
                     const std::map<dynd::nd::string, ndt::type> &tp_vars)
    {
      CallableType::get()->resolve_dst_type(CallableType::get()->static_data,
                                            data_size, data, dst_tp, nsrc,
                                            src_tp, kwds, tp_vars);
    }

    static intptr_t
    instantiate(char *DYND_UNUSED(static_data), size_t data_size, char *data,
                void *ckb, intptr_t ckb_offset, const ndt::type &dst_tp,
                const char *dst_arrmeta, intptr_t nsrc, const ndt::type *src_tp,
                const char *const *src_arrmeta, kernel_request_t kernreq,
                const eval::eval_context *ectx, const nd::array &kwds,
                const std::map<nd::string, ndt::type> &tp_vars)
    {
      return CallableType::get()->instantiate(
          CallableType::get()->static_data, data_size, data, ckb, ckb_offset,
          dst_tp, dst_arrmeta, nsrc, src_tp, src_arrmeta, kernreq, ectx, kwds,
          tp_vars);
    }
  };

} // namespace dynd::nd
} // namespace dynd