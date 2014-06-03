//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/type.hpp>
#include <dynd/types/base_expression_type.hpp>
#include <dynd/kernels/expression_comparison_kernels.hpp>
#include <dynd/kernels/assignment_kernels.hpp>

using namespace std;
using namespace dynd;

namespace {
    struct single_buffer {
        // Offset, from the start of &base, to the kernel for buffering
        size_t kernel_offset;
        const base_type *tp;
        char *arrmeta;
        size_t data_offset, data_size;
    };

    struct buffered_kernel_extra {
        typedef buffered_kernel_extra extra_type;

        ckernel_prefix base;
        // Offset, from the start of 'base' to the comparison kernel
        size_t cmp_kernel_offset;
        single_buffer buf[2];

        // Initializes the type and arrmeta for one of the two buffers
        // NOTE: This does NOT initialize buf[i].data_offset or buf[i].kernel_offset
        void init_buffer(int i, const ndt::type& buffer_dt_) {
            single_buffer& b = buf[i];
            // The kernel data owns a reference in buffer_dt
            b.tp = ndt::type(buffer_dt_).release();
            if (!buffer_dt_.is_builtin()) {
                size_t buffer_arrmeta_size = buffer_dt_.extended()->get_arrmeta_size();
                if (buffer_arrmeta_size > 0) {
                    b.arrmeta = reinterpret_cast<char *>(malloc(buffer_arrmeta_size));
                    if (b.arrmeta == NULL) {
                        throw bad_alloc();
                    }
                    b.tp->arrmeta_default_construct(b.arrmeta, 0, NULL);
                }
                // Make sure the buffer data size is pointer size-aligned
                b.data_size = inc_to_alignment(b.tp->get_default_data_size(0, NULL),
                                sizeof(void *));
            } else {
                // Make sure the buffer data size is pointer size-aligned
                b.data_size = inc_to_alignment(buffer_dt_.get_data_size(), sizeof(void *));
            }
        }

        inline const char *buffer_operand(const single_buffer& b, const char *src)
        {
            char *eraw = reinterpret_cast<char *>(this);
            char *dst = eraw + b.data_offset;

            // If the type needs it, initialize the buffer data to zero
            if (!is_builtin_type(b.tp) && (b.tp->get_flags()&type_flag_zeroinit) != 0) {
                memset(dst, 0, b.data_size);
            }

            // Get and execute the assignment kernel
            ckernel_prefix *echild;
            unary_single_operation_t opchild;
            echild = reinterpret_cast<ckernel_prefix *>(eraw + b.kernel_offset);
            opchild = echild->get_function<unary_single_operation_t>();
            opchild(dst, src, echild);

            // Return the buffer
            return dst;
        }

        static int kernel(const char *src0, const char *src1, ckernel_prefix *extra)
        {
            char *eraw = reinterpret_cast<char *>(extra);
            extra_type *e = reinterpret_cast<extra_type *>(extra);
            // Buffer the first operand if necessary
            if (e->buf[0].kernel_offset != 0) {
                src0 = e->buffer_operand(e->buf[0], src0);
            }
            // Buffer the second operand if necessary
            if (e->buf[1].kernel_offset != 0) {
                src1 = e->buffer_operand(e->buf[1], src1);
            }
            // Call the comparison kernel
            ckernel_prefix *echild;
            binary_single_predicate_t opchild;
            echild = reinterpret_cast<ckernel_prefix *>(eraw + e->cmp_kernel_offset);
            opchild = echild->get_function<binary_single_predicate_t>();
            int result = opchild(src0, src1, echild);

            // Clear the buffer data if necessary
            if (e->buf[0].arrmeta != NULL) {
                e->buf[0].tp->arrmeta_reset_buffers(e->buf[0].arrmeta);
            }
            if (e->buf[1].arrmeta != NULL) {
                e->buf[1].tp->arrmeta_reset_buffers(e->buf[1].arrmeta);
            }

            return result;
        }

        static void destruct(ckernel_prefix *self)
        {
            extra_type *e = reinterpret_cast<extra_type *>(self);

            for (int i = 0; i < 2; ++i) {
                single_buffer& b = e->buf[i];

                ndt::type dt(b.tp, false);
                // Steal the buffer0_tp reference count into an ndt::type
                char *arrmeta = b.arrmeta;
                // Destruct and free the arrmeta for the buffer
                if (arrmeta != NULL) {
                    dt.extended()->arrmeta_destruct(arrmeta);
                    free(arrmeta);
                }
                // Destruct the kernel for the buffer
                self->destroy_child_ckernel(b.kernel_offset);
            }

            // Destruct the comparison kernel
            self->destroy_child_ckernel(e->cmp_kernel_offset);
        }
    };
} // anonymous namespace

size_t dynd::make_expression_comparison_kernel(
                ckernel_builder *out, size_t offset_out,
                const ndt::type& src0_dt, const char *src0_arrmeta,
                const ndt::type& src1_dt, const char *src1_arrmeta,
                comparison_type_t comptype,
                const eval::eval_context *ectx)
{
    size_t current_offset = offset_out + sizeof(buffered_kernel_extra);
    out->ensure_capacity(current_offset);
    buffered_kernel_extra *e = out->get_at<buffered_kernel_extra>(offset_out);
    e->base.set_function<binary_single_predicate_t>(&buffered_kernel_extra::kernel);
    e->base.destructor = &buffered_kernel_extra::destruct;
    // Initialize the information for buffering the operands
    if (src0_dt.get_kind() == expression_kind) {
        e->init_buffer(0, src0_dt.value_type());
        e->buf[0].kernel_offset = current_offset - offset_out;
        current_offset = make_assignment_kernel(out, current_offset,
                        src0_dt.value_type(), e->buf[0].arrmeta,
                        src0_dt, src0_arrmeta,
                        kernel_request_single, assign_error_none, ectx);
        // Have to re-retrieve 'e', because creating another kernel may invalidate it
        e = out->get_at<buffered_kernel_extra>(offset_out);
    }
    if (src1_dt.get_kind() == expression_kind) {
        e->init_buffer(1, src1_dt.value_type());
        e->buf[1].kernel_offset = current_offset - offset_out;
        current_offset = make_assignment_kernel(out, current_offset,
                        src1_dt.value_type(), e->buf[1].arrmeta,
                        src1_dt, src1_arrmeta,
                        kernel_request_single, assign_error_none, ectx);
        // Have to re-retrieve 'e', because creating another kernel may invalidate it
        e = out->get_at<buffered_kernel_extra>(offset_out);
    }
    // Allocate the data for the buffers
    if (e->buf[0].kernel_offset != 0) {
        current_offset = inc_to_alignment(current_offset, src0_dt.get_data_alignment());
        e->buf[0].data_offset = current_offset - offset_out;
        current_offset += e->buf[0].data_size;
    }
    if (e->buf[1].kernel_offset != 0) {
        current_offset = inc_to_alignment(current_offset, src1_dt.get_data_alignment());
        e->buf[1].data_offset = current_offset - offset_out;
        current_offset += e->buf[1].data_size;
    }
    out->ensure_capacity(current_offset);
    // Have to re-retrieve 'e', because allocating the buffer data may invalidate it
    e = out->get_at<buffered_kernel_extra>(offset_out);
    e->cmp_kernel_offset = current_offset - offset_out;
    return make_comparison_kernel(out, current_offset,
                    src0_dt.value_type(),
                    (e->buf[0].kernel_offset != 0) ? e->buf[0].arrmeta : src0_arrmeta,
                    src1_dt.value_type(),
                    (e->buf[1].kernel_offset != 0) ? e->buf[1].arrmeta : src1_arrmeta,
                    comptype, ectx);
}
