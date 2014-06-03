//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/kernels/date_assignment_kernels.hpp>
#include <dynd/kernels/assignment_kernels.hpp>
#include <dynd/types/cstruct_type.hpp>

using namespace std;
using namespace dynd;

/////////////////////////////////////////
// string to date assignment

namespace {
    struct string_to_date_ck : public kernels::assignment_ck<string_to_date_ck> {
        ndt::type m_src_string_tp;
        const char *m_src_arrmeta;
        assign_error_mode m_errmode;
        date_parse_order_t m_date_parse_order;
        int m_century_window;

        inline void single(char *dst, const char *src)
        {
            const base_string_type *bst = static_cast<const base_string_type *>(m_src_string_tp.extended());
            const string& s = bst->get_utf8_string(m_src_arrmeta, src, m_errmode);
            date_ymd ymd;
            // TODO: properly distinguish "date" and "option[date]" with respect to NA support
            if (s == "NA") {
                ymd.set_to_na();
            } else {
                ymd.set_from_str(s, m_date_parse_order, m_century_window);
            }
            *reinterpret_cast<int32_t *>(dst) = ymd.to_days();
        }
    };
} // anonymous namespace

size_t dynd::make_string_to_date_assignment_kernel(
                ckernel_builder *out_ckb, size_t ckb_offset,
                const ndt::type& src_string_tp, const char *src_arrmeta,
                kernel_request_t kernreq, assign_error_mode errmode,
                const eval::eval_context *ectx)
{
    typedef string_to_date_ck self_type;
    if (src_string_tp.get_kind() != string_kind) {
        stringstream ss;
        ss << "make_string_to_date_assignment_kernel: source type " << src_string_tp << " is not a string type";
        throw runtime_error(ss.str());
    }

    self_type *self = self_type::create_leaf(out_ckb, ckb_offset, kernreq);
    self->m_src_string_tp = src_string_tp;
    self->m_src_arrmeta = src_arrmeta;
    self->m_errmode = errmode;
    self->m_date_parse_order = ectx->date_parse_order;
    self->m_century_window = ectx->century_window;
    return ckb_offset + sizeof(self_type);
}

/////////////////////////////////////////
// date to string assignment

namespace {
    struct date_to_string_ck : public kernels::assignment_ck<date_to_string_ck> {
        ndt::type m_dst_string_tp;
        const char *m_dst_arrmeta;
        assign_error_mode m_errmode;

        inline void single(char *dst, const char *src)
        {
            date_ymd ymd;
            ymd.set_from_days(*reinterpret_cast<const int32_t *>(src));
            string s = ymd.to_str();
            if (s.empty()) {
                s = "NA";
            }
            const base_string_type *bst = static_cast<const base_string_type *>(m_dst_string_tp.extended());
            bst->set_utf8_string(m_dst_arrmeta, dst, m_errmode, s);
        }
    };
} // anonymous namespace

size_t dynd::make_date_to_string_assignment_kernel(
                ckernel_builder *out_ckb, size_t ckb_offset,
                const ndt::type& dst_string_tp, const char *dst_arrmeta,
                kernel_request_t kernreq, assign_error_mode errmode,
                const eval::eval_context *DYND_UNUSED(ectx))
{
    typedef date_to_string_ck self_type;
    if (dst_string_tp.get_kind() != string_kind) {
        stringstream ss;
        ss << "get_date_to_string_assignment_kernel: dest type " << dst_string_tp << " is not a string type";
        throw runtime_error(ss.str());
    }

    self_type *self = self_type::create_leaf(out_ckb, ckb_offset, kernreq);
    self->m_dst_string_tp = dst_string_tp;
    self->m_dst_arrmeta = dst_arrmeta;
    self->m_errmode = errmode;
    return ckb_offset + sizeof(self_type);
}

