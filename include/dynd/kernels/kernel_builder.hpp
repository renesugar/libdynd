//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <new>
#include <algorithm>
#include <map>

#include <dynd/kernels/ckernel_prefix.hpp>

namespace dynd {

/**
 * Aligns an offset as required by ckernels.
 */
inline size_t align_offset(size_t offset) { return (offset + size_t(7)) & ~size_t(7); }

namespace nd {

  /**
   * Function pointers + data for a hierarchical
   * kernel which operates on type/arrmeta in
   * some configuration.
   *
   * The data placed in the kernel's data must
   * be relocatable with a memcpy, it must not rely on its
   * own address.
   */
  class kernel_builder {
  public:
    intptr_t m_size;

  protected:
    // Pointer to the kernel function pointers + data
    char *m_data;
    intptr_t m_capacity;

    // When the amount of data is small, this static data is used,
    // otherwise dynamic memory is allocated when it gets too big
    char m_static_data[16 * 8];

    bool using_static_data() const { return m_data == &m_static_data[0]; }

    void init()
    {
      m_data = &m_static_data[0];
      m_size = 0;
      m_capacity = sizeof(m_static_data);
      set(m_static_data, 0, sizeof(m_static_data));
    }

    void destroy()
    {
      if (m_data != NULL) {
        // Destroy whatever was created
        destroy(reinterpret_cast<ckernel_prefix *>(m_data));
        // Free the memory
        free(m_data);
      }
    }

  public:
    kernel_builder() { init(); }

    ~kernel_builder() { destroy(); }

    void destroy(ckernel_prefix *self) { self->destroy(); }

    void reset()
    {
      destroy();
      init();
    }

    /**
     * This function ensures that the ckernel's data
     * is at least the required number of bytes. It
     * should only be called during the construction phase
     * of the kernel when constructing a leaf kernel.
     */
    void reserve(intptr_t requested_capacity)
    {
      if (m_capacity < requested_capacity) {
        // Grow by a factor of 1.5
        // https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md
        intptr_t grown_capacity = m_capacity * 3 / 2;
        if (requested_capacity < grown_capacity) {
          requested_capacity = grown_capacity;
        }
        // Do a realloc
        char *new_data = reinterpret_cast<char *>(realloc(m_data, m_capacity, requested_capacity));
        if (new_data == NULL) {
          destroy();
          m_data = NULL;
          throw std::bad_alloc();
        }
        // Zero out the newly allocated capacity
        set(reinterpret_cast<char *>(new_data) + m_capacity, 0, requested_capacity - m_capacity);
        m_data = new_data;
        m_capacity = requested_capacity;
      }
    }

    ckernel_prefix *get() const { return reinterpret_cast<ckernel_prefix *>(m_data); }

    /**
     * For use during construction, gets the ckernel component
     * at the requested offset.
     */
    template <typename KernelType>
    KernelType *get_at(size_t offset)
    {
      return reinterpret_cast<KernelType *>(m_data + offset);
    }

    void *alloc(size_t size) { return std::malloc(size); }

    void *realloc(void *ptr, size_t old_size, size_t new_size)
    {
      if (using_static_data()) {
        // If we were previously using the static data, do a malloc
        void *new_data = alloc(new_size);
        // If the allocation succeeded, copy the old data as the realloc would
        if (new_data != NULL) {
          copy(new_data, ptr, old_size);
        }
        return new_data;
      }
      else {
        return std::realloc(ptr, new_size);
      }
    }

    void free(void *ptr)
    {
      if (!using_static_data()) {
        std::free(ptr);
      }
    }

    void *copy(void *dst, const void *src, size_t size) { return std::memcpy(dst, src, size); }

    void *set(void *dst, int value, size_t size) { return std::memset(dst, value, size); }

    /** For debugging/informational purposes */
    intptr_t get_capacity() const { return m_capacity; }

    /**
     * Creates the kernel, and increments ``m_size`` to the position after it.
     */
    template <typename KernelType, typename... ArgTypes>
    void emplace_back(ArgTypes &&... args)
    {
      /* Alignment requirement of the type. */
      static_assert(alignof(KernelType) <= 8, "kernel types require alignment <= 64 bits");

      intptr_t ckb_offset = m_size;
      m_size +=
          align_offset(sizeof(KernelType)); // The increment needs to be aligned to 8 bytes, so padding may be added.
      reserve(m_size);
      KernelType::init(this->get_at<KernelType>(ckb_offset), std::forward<ArgTypes>(args)...);
    }
  };

} // namespace dynd::nd

inline void ckernel_prefix::instantiate(char *static_data, char *DYND_UNUSED(data), nd::kernel_builder *ckb,
                                        intptr_t DYND_UNUSED(ckb_offset), const ndt::type &DYND_UNUSED(dst_tp),
                                        const char *DYND_UNUSED(dst_arrmeta), intptr_t DYND_UNUSED(nsrc),
                                        const ndt::type *DYND_UNUSED(src_tp),
                                        const char *const *DYND_UNUSED(src_arrmeta), kernel_request_t kernreq,
                                        intptr_t DYND_UNUSED(nkwd), const nd::array *DYND_UNUSED(kwds),
                                        const std::map<std::string, ndt::type> &DYND_UNUSED(tp_vars))
{
  void *func;
  switch (kernreq) {
  case kernel_request_single:
    func = reinterpret_cast<kernel_targets_t *>(static_data)->single;
    break;
  case kernel_request_strided:
    func = reinterpret_cast<kernel_targets_t *>(static_data)->strided;
    break;
  default:
    throw std::invalid_argument("unrecognized kernel request");
    break;
  }

  if (func == NULL) {
    throw std::invalid_argument("no kernel request");
  }

  ckb->emplace_back<ckernel_prefix>(func);
}

} // namespace dynd
