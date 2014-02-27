//
// Copyright (C) 2011-14 Irwin Zaid, Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/types/base_memory_type.hpp>
#include <dynd/gfunc/make_callable.hpp>

using namespace std;
using namespace dynd;

base_memory_type::~base_memory_type()
{
}

size_t base_memory_type::get_default_data_size(intptr_t ndim, const intptr_t *shape) const {
    if (m_target_tp.is_builtin()) {
        return m_target_tp.get_data_size();
    } else {
        return m_target_tp.extended()->get_default_data_size(ndim, shape);
    }
}

void base_memory_type::print_data(std::ostream& o, const char *metadata, const char *data) const
{
    m_target_tp.print_data(o, metadata + m_target_metadata_offset, data);
}

bool base_memory_type::is_lossless_assignment(const ndt::type& dst_tp, const ndt::type& src_tp) const
{
    // Default to calling with the target types
    if (dst_tp.extended() == this) {
        return ::is_lossless_assignment(m_target_tp, src_tp);
    } else {
        return ::is_lossless_assignment(dst_tp, m_target_tp);
    }
}

bool base_memory_type::operator==(const base_type& rhs) const
{
    // Default to equivalence between the target types
    if (this == &rhs) {
        return true;
    } else if (rhs.get_kind() != memory_kind) {
        return false;
    } else {
        const base_memory_type *dt = static_cast<const base_memory_type*>(&rhs);
        return m_target_tp == dt->m_target_tp;
    }
}

void base_memory_type::transform_child_types(type_transform_fn_t transform_fn, void *extra,
                ndt::type& out_transformed_tp, bool& out_was_transformed) const
{
    ndt::type tmp_tp;
    bool was_transformed = false;
    transform_fn(m_target_tp, extra, tmp_tp, was_transformed);
    if (was_transformed) {
        out_transformed_tp = with_replaced_target_type(tmp_tp);
        out_was_transformed = true;
    } else {
        out_transformed_tp = ndt::type(this, true);
    }
}

ndt::type base_memory_type::get_canonical_type() const
{
    return m_target_tp.get_canonical_type();
}

void base_memory_type::metadata_default_construct(char *metadata, intptr_t ndim, const intptr_t* shape) const
{
    if (!m_target_tp.is_builtin()) {
        m_target_tp.extended()->metadata_default_construct(metadata + m_target_metadata_offset, ndim, shape);
    }
}

void base_memory_type::metadata_copy_construct(char *dst_metadata, const char *src_metadata, memory_block_data *embedded_reference) const
{
    if (!m_target_tp.is_builtin()) {
        m_target_tp.extended()->metadata_copy_construct(dst_metadata + m_target_metadata_offset,
                        src_metadata + m_target_metadata_offset, embedded_reference);
    }
}

void base_memory_type::metadata_destruct(char *metadata) const
{
    if (!m_target_tp.is_builtin()) {
        m_target_tp.extended()->metadata_destruct(metadata + m_target_metadata_offset);
    }
}

static ndt::type property_get_target_type(const ndt::type& tp) {
    const base_memory_type *md = static_cast<const base_memory_type *>(tp.extended());
    return md->get_target_type();
}

static pair<string, gfunc::callable> type_properties[] = {
    pair<string, gfunc::callable>("target_type", gfunc::make_callable(&property_get_target_type, "self"))
};

void base_memory_type::get_dynamic_type_properties(
                const std::pair<std::string, gfunc::callable> **out_properties,
                size_t *out_count) const
{
    *out_properties = type_properties;
    *out_count = sizeof(type_properties) / sizeof(type_properties[0]);
}
