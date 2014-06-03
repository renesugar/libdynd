//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/type.hpp>
#include <dynd/types/base_struct_type.hpp>
#include <dynd/types/struct_type.hpp>
#include <dynd/types/string_type.hpp>
#include <dynd/types/type_type.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/shortvector.hpp>
#include <dynd/shape_tools.hpp>

using namespace std;
using namespace dynd;

base_struct_type::base_struct_type(type_id_t type_id,
                                   const nd::array &field_names,
                                   const nd::array &field_types,
                                   flags_type flags, bool variable_layout)
    : base_tuple_type(type_id, field_types, flags, variable_layout),
      m_field_names(field_names)
{
    if (!nd::ensure_immutable_contig<nd::string>(m_field_names)) {
        stringstream ss;
        ss << "dynd struct field names requires an array of strings, got an "
              "array with type " << m_field_names.get_type();
        throw invalid_argument(ss.str());
    }

    // Make sure that the number of names matches
    size_t name_count = reinterpret_cast<const strided_dim_type_arrmeta *>(
                            m_field_names.get_arrmeta())->size;
    if (name_count != m_field_count) {
        stringstream ss;
        ss << "dynd struct type requires that the number of names, "
           << name_count << " matches the number of types, " << m_field_count;
        throw invalid_argument(ss.str());
    }

    m_members.kind = struct_kind;
}

base_struct_type::~base_struct_type() {
}

intptr_t base_struct_type::get_field_index(const char *field_name_begin,
                                           const char *field_name_end) const
{
    size_t size = field_name_end - field_name_begin;
    if (size > 0) {
        char firstchar = *field_name_begin;
        size_t field_count = get_field_count();
        const char *fn_ptr = m_field_names.get_readonly_originptr();
        intptr_t fn_stride =
            reinterpret_cast<const strided_dim_type_arrmeta *>(
                m_field_names.get_arrmeta())->stride;
        for (size_t i = 0; i != field_count; ++i, fn_ptr += fn_stride) {
            const string_type_data *fn = reinterpret_cast<const string_type_data *>(fn_ptr);
            const char *begin = fn->begin, *end = fn->end;
            if ((size_t)(end - begin) == size && *begin == firstchar) {
                if (memcmp(fn->begin, field_name_begin, size) == 0) {
                    return i;
                }
            }
        }
    }

    return -1;
}

ndt::type base_struct_type::apply_linear_index(intptr_t nindices,
                                               const irange *indices,
                                               size_t current_i,
                                               const ndt::type &root_tp,
                                               bool leading_dimension) const
{
    if (nindices == 0) {
        return ndt::type(this, true);
    } else {
        bool remove_dimension;
        intptr_t start_index, index_stride, dimension_size;
        apply_single_linear_index(*indices, m_field_count, current_i, &root_tp,
                                  remove_dimension, start_index, index_stride,
                                  dimension_size);
        if (remove_dimension) {
            return get_field_type(start_index)
                .apply_linear_index(nindices - 1, indices + 1, current_i + 1,
                                    root_tp, leading_dimension);
        } else if (nindices == 1 && start_index == 0 && index_stride == 1 &&
                        (size_t)dimension_size == m_field_count) {
            // This is a do-nothing index, keep the same type
            return ndt::type(this, true);
        } else {
            // Take the subset of the fields in-place
            nd::array tmp_field_types(nd::empty(
                dimension_size, ndt::make_strided_of_type()));
            ndt::type *tmp_field_types_raw = reinterpret_cast<ndt::type *>(
                tmp_field_types.get_readwrite_originptr());

            // Make a "strided * string" array without copying the actual
            // string text data. TODO: encapsulate this into a function.
            char *data_ptr;
            string_type_data *string_arr_ptr;
            ndt::type stp = ndt::make_string(string_encoding_utf_8);
            ndt::type tp = ndt::make_strided_dim(stp);
            nd::array tmp_field_names(
                make_array_memory_block(tp.extended()->get_arrmeta_size(),
                                        dimension_size * stp.get_data_size(),
                                        tp.get_data_alignment(), &data_ptr));
            // Set the array arrmeta
            array_preamble *ndo = tmp_field_names.get_ndo();
            ndo->m_type = tp.release();
            ndo->m_data_pointer = data_ptr;
            ndo->m_data_reference = NULL;
            ndo->m_flags = nd::default_access_flags;
            string_arr_ptr = reinterpret_cast<string_type_data *>(data_ptr);
            // Get the allocator for the output string type
            strided_dim_type_arrmeta *md =
                reinterpret_cast<strided_dim_type_arrmeta *>(
                    tmp_field_names.get_arrmeta());
            md->size = dimension_size;
            md->stride = stp.get_data_size();
            string_type_arrmeta *smd =
                reinterpret_cast<string_type_arrmeta *>(
                    tmp_field_names.get_arrmeta() +
                    sizeof(strided_dim_type_arrmeta));
            const string_type_arrmeta *smd_orig =
                reinterpret_cast<const string_type_arrmeta *>(
                    m_field_names.get_arrmeta() +
                    sizeof(strided_dim_type_arrmeta));
            smd->blockref = smd_orig->blockref
                                ? smd_orig->blockref
                                : m_field_names.get_memblock().get();
            memory_block_incref(smd->blockref);

            for (intptr_t i = 0; i < dimension_size; ++i)
            {
                intptr_t idx = start_index + i * index_stride;
                tmp_field_types_raw[i] = get_field_type(idx).apply_linear_index(
                    nindices - 1, indices + 1, current_i + 1, root_tp, false);
                string_arr_ptr[i] = get_field_name_raw(idx);
            }

            tmp_field_types.flag_as_immutable();
            return ndt::make_struct(tmp_field_names, tmp_field_types);
        }
    }
}

intptr_t base_struct_type::apply_linear_index(
    intptr_t nindices, const irange *indices, const char *arrmeta,
    const ndt::type &result_tp, char *out_arrmeta,
    memory_block_data *embedded_reference, size_t current_i,
    const ndt::type &root_tp, bool leading_dimension, char **inout_data,
    memory_block_data **inout_dataref) const
{
    if (nindices == 0) {
        // If there are no more indices, copy the arrmeta verbatim
        arrmeta_copy_construct(out_arrmeta, arrmeta, embedded_reference);
        return 0;
    } else {
        const uintptr_t *offsets = get_data_offsets(arrmeta);
        const uintptr_t *arrmeta_offsets = get_arrmeta_offsets_raw();
        bool remove_dimension;
        intptr_t start_index, index_stride, dimension_size;
        apply_single_linear_index(*indices, m_field_count, current_i, &root_tp,
                        remove_dimension, start_index, index_stride, dimension_size);
        if (remove_dimension) {
            const ndt::type& dt = get_field_type(start_index);
            intptr_t offset = offsets[start_index];
            if (!dt.is_builtin()) {
                if (leading_dimension) {
                    // In the case of a leading dimension, first bake the offset into
                    // the data pointer, so that it's pointing at the right element
                    // for the collapsing of leading dimensions to work correctly.
                    *inout_data += offset;
                    offset = dt.extended()->apply_linear_index(nindices - 1, indices + 1,
                                    arrmeta + arrmeta_offsets[start_index], result_tp,
                                    out_arrmeta, embedded_reference, current_i + 1, root_tp,
                                    true, inout_data, inout_dataref);
                } else {
                    offset += dt.extended()->apply_linear_index(nindices - 1, indices + 1,
                                    arrmeta + arrmeta_offsets[start_index], result_tp,
                                    out_arrmeta, embedded_reference, current_i + 1, root_tp,
                                    false, NULL, NULL);
                }
            }
            return offset;
        } else {
            intptr_t *out_offsets = reinterpret_cast<intptr_t *>(out_arrmeta);
            const struct_type *result_e_dt = result_tp.tcast<struct_type>();
            for (intptr_t i = 0; i < dimension_size; ++i) {
                intptr_t idx = start_index + i * index_stride;
                out_offsets[i] = offsets[idx];
                const ndt::type& dt = result_e_dt->get_field_type(i);
                if (!dt.is_builtin()) {
                    out_offsets[i] += dt.extended()->apply_linear_index(nindices - 1, indices + 1,
                                    arrmeta + arrmeta_offsets[idx], dt,
                                    out_arrmeta + result_e_dt->get_arrmeta_offset(i),
                                    embedded_reference, current_i + 1, root_tp,
                                    false, NULL, NULL);
                }
            }
            return 0;
        }
    }
}

size_t base_struct_type::get_elwise_property_index(const std::string& property_name) const
{
    intptr_t i = get_field_index(property_name);
    if (i >= 0) {
        return i;
    } else {
        stringstream ss;
        ss << "dynd type " << ndt::type(this, true) << " does not have a kernel for property " << property_name;
        throw runtime_error(ss.str());
    }
}

ndt::type base_struct_type::get_elwise_property_type(size_t elwise_property_index,
                bool& out_readable, bool& out_writable) const
{
    size_t field_count = get_field_count();
    if (elwise_property_index < field_count) {
        out_readable = true;
        out_writable = false;
        return get_field_type(elwise_property_index).value_type();
    } else {
        return ndt::make_type<void>();
    }
}

namespace {
    struct struct_property_getter_extra {
        typedef struct_property_getter_extra extra_type;

        ckernel_prefix base;
        size_t field_offset;

        static void single(char *dst, const char *src, ckernel_prefix *extra)
        {
            extra_type *e = reinterpret_cast<extra_type *>(extra);
            ckernel_prefix *echild = &(e + 1)->base;
            unary_single_operation_t opchild = echild->get_function<unary_single_operation_t>();
            opchild(dst, src + e->field_offset, echild);
        }
        static void strided(char *dst, intptr_t dst_stride,
                        const char *src, intptr_t src_stride,
                        size_t count, ckernel_prefix *extra)
        {
            extra_type *e = reinterpret_cast<extra_type *>(extra);
            ckernel_prefix *echild = &(e + 1)->base;
            unary_strided_operation_t opchild = echild->get_function<unary_strided_operation_t>();
            opchild(dst, dst_stride, src + e->field_offset, src_stride, count, echild);
        }

        static void destruct(ckernel_prefix *self)
        {
            self->destroy_child_ckernel(sizeof(extra_type));
        }
    };
} // anonymous namespace

size_t base_struct_type::make_elwise_property_getter_kernel(
                ckernel_builder *out, size_t offset_out,
                const char *dst_arrmeta,
                const char *src_arrmeta, size_t src_elwise_property_index,
                kernel_request_t kernreq, const eval::eval_context *ectx) const
{
    size_t field_count = get_field_count();
    if (src_elwise_property_index < field_count) {
        const uintptr_t *arrmeta_offsets = get_arrmeta_offsets_raw();
        const ndt::type& field_type = get_field_type(src_elwise_property_index);
        out->ensure_capacity(offset_out + sizeof(struct_property_getter_extra));
        struct_property_getter_extra *e = out->get_at<struct_property_getter_extra>(offset_out);
        switch (kernreq) {
            case kernel_request_single:
                e->base.set_function<unary_single_operation_t>(&struct_property_getter_extra::single);
                break;
            case kernel_request_strided:
                e->base.set_function<unary_strided_operation_t>(&struct_property_getter_extra::strided);
                break;
            default: {
                stringstream ss;
                ss << "base_struct_type::make_elwise_property_getter_kernel: ";
                ss << "unrecognized request " << (int)kernreq;
                throw runtime_error(ss.str());
            }   
        }
        e->base.destructor = &struct_property_getter_extra::destruct;
        e->field_offset = get_data_offsets(src_arrmeta)[src_elwise_property_index];
        return ::make_assignment_kernel(out, offset_out + sizeof(struct_property_getter_extra),
                        field_type.value_type(), dst_arrmeta,
                        field_type, src_arrmeta + arrmeta_offsets[src_elwise_property_index],
                        kernreq, assign_error_none, ectx);
    } else {
        stringstream ss;
        ss << "dynd type " << ndt::type(this, true);
        ss << " given an invalid property index" << src_elwise_property_index;
        throw runtime_error(ss.str());
    }
}

size_t base_struct_type::make_elwise_property_setter_kernel(
                ckernel_builder *DYND_UNUSED(out), size_t DYND_UNUSED(offset_out),
                const char *DYND_UNUSED(dst_arrmeta), size_t dst_elwise_property_index,
                const char *DYND_UNUSED(src_arrmeta),
                kernel_request_t DYND_UNUSED(kernreq), const eval::eval_context *DYND_UNUSED(ectx)) const
{
    // No writable properties
    stringstream ss;
    ss << "dynd type " << ndt::type(this, true);
    ss << " given an invalid property index" << dst_elwise_property_index;
    throw runtime_error(ss.str());
}

