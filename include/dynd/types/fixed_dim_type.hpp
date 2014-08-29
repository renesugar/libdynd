//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__FIXED_DIM_TYPE_HPP_
#define _DYND__FIXED_DIM_TYPE_HPP_

#include <dynd/type.hpp>
#include <dynd/types/base_dim_type.hpp>
#include <dynd/typed_data_assign.hpp>
#include <dynd/types/view_type.hpp>

namespace dynd {

// fixed_dim (redundantly) uses the same arrmeta as strided_dim
typedef size_stride_t fixed_dim_type_arrmeta;

struct fixed_dim_type_iterdata {
    iterdata_common common;
    char *data;
    intptr_t stride;
};

class fixed_dim_type : public base_dim_type {
    intptr_t m_dim_size;
    std::vector<std::pair<std::string, gfunc::callable> > m_array_properties, m_array_functions;
public:
    fixed_dim_type(intptr_t dim_size, const ndt::type& element_tp);

    virtual ~fixed_dim_type();

    size_t get_default_data_size(intptr_t ndim, const intptr_t *shape) const;

    inline intptr_t get_fixed_dim_size() const {
        return m_dim_size;
    }

    void print_data(std::ostream& o, const char *arrmeta, const char *data) const;

    void print_type(std::ostream& o) const;

    bool is_expression() const;
    bool is_unique_data_owner(const char *arrmeta) const;
    void transform_child_types(type_transform_fn_t transform_fn, void *extra,
                    ndt::type& out_transformed_tp, bool& out_was_transformed) const;
    ndt::type get_canonical_type() const;

    ndt::type apply_linear_index(intptr_t nindices, const irange *indices,
                size_t current_i, const ndt::type& root_tp, bool leading_dimension) const;
    intptr_t apply_linear_index(intptr_t nindices, const irange *indices, const char *arrmeta,
                    const ndt::type& result_tp, char *out_arrmeta,
                    memory_block_data *embedded_reference,
                    size_t current_i, const ndt::type& root_tp,
                    bool leading_dimension, char **inout_data,
                    memory_block_data **inout_dataref) const;
    ndt::type at_single(intptr_t i0, const char **inout_arrmeta, const char **inout_data) const;

    ndt::type get_type_at_dimension(char **inout_arrmeta, intptr_t i, intptr_t total_ndim = 0) const;

    intptr_t get_dim_size(const char *arrmeta, const char *data) const;
        void get_shape(intptr_t ndim, intptr_t i, intptr_t *out_shape, const char *arrmeta, const char *data) const;
    void get_strides(size_t i, intptr_t *out_strides, const char *arrmeta) const;

    axis_order_classification_t classify_axis_order(const char *arrmeta) const;

    bool is_lossless_assignment(const ndt::type& dst_tp, const ndt::type& src_tp) const;

    bool operator==(const base_type& rhs) const;

    void arrmeta_default_construct(char *arrmeta, intptr_t ndim,
                                   const intptr_t *shape,
                                   bool blockref_alloc) const;
    void arrmeta_copy_construct(char *dst_arrmeta, const char *src_arrmeta,
                                memory_block_data *embedded_reference) const;
    void arrmeta_reset_buffers(char *arrmeta) const;
    void arrmeta_finalize_buffers(char *arrmeta) const;
    void arrmeta_destruct(char *arrmeta) const;
    void arrmeta_debug_print(const char *arrmeta, std::ostream &o,
                             const std::string &indent) const;
    size_t
    arrmeta_copy_construct_onedim(char *dst_arrmeta, const char *src_arrmeta,
                                  memory_block_data *embedded_reference) const;

    size_t get_iterdata_size(intptr_t ndim) const;
    size_t iterdata_construct(iterdata_common *iterdata, const char **inout_arrmeta, intptr_t ndim, const intptr_t* shape, ndt::type& out_uniform_tp) const;
    size_t iterdata_destruct(iterdata_common *iterdata, intptr_t ndim) const;

    void data_destruct(const char *arrmeta, char *data) const;
    void data_destruct_strided(const char *arrmeta, char *data,
                    intptr_t stride, size_t count) const;

    size_t make_assignment_kernel(ckernel_builder *ckb, intptr_t ckb_offset,
                                  const ndt::type &dst_tp,
                                  const char *dst_arrmeta,
                                  const ndt::type &src_tp,
                                  const char *src_arrmeta,
                                  kernel_request_t kernreq,
                                  const eval::eval_context *ectx) const;

    void foreach_leading(const char *arrmeta, char *data,
                         foreach_fn_t callback, void *callback_data) const;

    /**
     * Modifies arrmeta allocated using the arrmeta_default_construct function, to be used
     * immediately after nd::array construction. Given an input type/arrmeta, edits the output
     * arrmeta in place to match.
     *
     * \param dst_arrmeta  The arrmeta created by arrmeta_default_construct, which is modified in place
     * \param src_tp  The type of the input nd::array whose stride ordering is to be matched.
     * \param src_arrmeta  The arrmeta of the input nd::array whose stride ordering is to be matched.
     */
    void reorder_default_constructed_strides(
                    char *dst_arrmeta,
                    const ndt::type& src_tp, const char *src_arrmeta) const;

    void get_dynamic_type_properties(
                    const std::pair<std::string, gfunc::callable> **out_properties,
                    size_t *out_count) const;
    void get_dynamic_array_properties(
                    const std::pair<std::string, gfunc::callable> **out_properties,
                    size_t *out_count) const;
    void get_dynamic_array_functions(
                    const std::pair<std::string, gfunc::callable> **out_functions,
                    size_t *out_count) const;
};

namespace ndt {
    inline ndt::type make_fixed_dim(size_t dim_size, const ndt::type& element_tp) {
        return ndt::type(new fixed_dim_type(dim_size, element_tp), false);
    }

    ndt::type make_fixed_dim(intptr_t ndim, const intptr_t *shape,
                              const ndt::type &dtp);


    template<class T> struct fixed_dim_from_array {
        static inline ndt::type make() {
            return ndt::make_type<T>();
        }
    };
    template<class T, int N> struct fixed_dim_from_array<T[N]> {
        static inline ndt::type make() {
            return ndt::make_fixed_dim(N, ndt::fixed_dim_from_array<T>::make());
        }
    };
} // namespace ndt

} // namespace dynd

#endif // _DYND__FIXED_DIM_TYPE_HPP_
