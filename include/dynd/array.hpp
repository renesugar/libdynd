//
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__NDOBJECT_HPP_
#define _DYND__NDOBJECT_HPP_

#include <iostream> // FOR DEBUG
#include <stdexcept>
#include <string>

#include <dynd/config.hpp>

#include <dynd/dtype.hpp>
#include <dynd/dtype_assign.hpp>
#include <dynd/shortvector.hpp>
#include <dynd/irange.hpp>
#include <dynd/memblock/array_memory_block.hpp>

namespace dynd { namespace nd {

class array;

enum array_access_flags {
    /** If an ndarray node is readable */
    read_access_flag = 0x01,
    /** If an ndarray node is writeable */
    write_access_flag = 0x02,
    /** If an ndarray node will not be written to by anyone else either */
    immutable_access_flag = 0x04
};

/** Stream printing function */
std::ostream& operator<<(std::ostream& o, const array& rhs);

class array_vals;
class array_vals_at;

/**
 * This is the primary multi-dimensional array class.
 */
class array {
    /**
     * The nd::array class is a wrapper around an array_memory_block, which
     * contains metadata as described by the dtype.
     */
    memory_block_ptr m_memblock;

    // Don't allow implicit construction from a raw pointer
    array(const void *);
public:
    /** Constructs an array with no buffer (NULL state) */
    array();
    /**
     * Constructs a zero-dimensional scalar from a C++ scalar.
     *
     */
    array(dynd_bool value);
    array(bool value);
    array(signed char value);
    array(short value);
    array(int value);
    array(long value);
    array(long long value);
    array(const dynd_int128& value);
    array(unsigned char value);
    array(unsigned short value);
    array(unsigned int value);
    array(unsigned long value);
    array(unsigned long long value);
    array(const dynd_uint128& value);
    array(dynd_float16 value);
    array(float value);
    array(double value);
    array(const dynd_float128& value);
    array(std::complex<float> value);
    array(std::complex<double> value);
    array(const std::string& value);
    /** Construct a string from a NULL-terminated UTF8 string */
    array(const char *cstr);
    /** Construct a string from a UTF8 buffer and specified buffer size */
    array(const char *str, size_t size);
    /**
     * Constructs a scalar with the 'dtype' dtype.
     * NOTE: Does NOT create a scalar of the provided dtype,
     *       use dynd::empty(dtype) for that!
     */
    array(const dtype& dt);

    /**
     * Constructs an array from a multi-dimensional C-style array.
     */
    template<class T, int N>
    array(const T (&rhs)[N]);
    /** Specialize to treat char arrays as strings */
    template<int N>
    array(const char (&rhs)[N]);
    /** Specialize to create 1D arrays of strings */
    template<int N>
    array(const char *(&rhs)[N]);

    /**
     * Constructs an array from a std::vector.
     */
    template<class T>
    array(const std::vector<T>& vec);

    explicit array(const memory_block_ptr& ndobj_memblock)
        : m_memblock(ndobj_memblock)
    {
        if (m_memblock.get()->m_type != array_memory_block_type) {
            throw std::runtime_error("array can only be constructed from a memblock with array type");
        }
    }

    explicit array(array_preamble *ndo, bool add_ref)
        : m_memblock(&ndo->m_memblockdata, add_ref)
    {}

    void set(const memory_block_ptr& ndobj_memblock)
    {
        if (ndobj_memblock.get()->m_type != array_memory_block_type) {
            throw std::runtime_error("array can only be constructed from a memblock with array type");
        }
        m_memblock = ndobj_memblock;
    }

#ifdef DYND_RVALUE_REFS
    void set(memory_block_ptr&& ndobj_memblock)
    {
        if (ndobj_memblock.get()->m_type != array_memory_block_type) {
            throw std::runtime_error("array can only be constructed from a memblock with array type");
        }
        m_memblock = DYND_MOVE(ndobj_memblock);
    }
#endif

    // TODO: Copy the initializer list mechanisms from ndarray

    /** Swap operation (should be "noexcept" in C++11) */
    void swap(array& rhs);

    /**
     * Assignment operator (should be just "= default" in C++11).
     * Copies with reference semantics.
     */
    inline array& operator=(const array& rhs) {
        m_memblock = rhs.m_memblock;
        return *this;
    }
#ifdef DYND_RVALUE_REFS
    /** Move assignment operator (should be just "= default" in C++11) */
    inline array& operator=(array&& rhs) {
        m_memblock = DYND_MOVE(rhs.m_memblock);

        return *this;
    }
#endif // DYND_RVALUE_REFS

    /**
     * This function releases the memory block reference, setting the
     * array to NULL. The caller takes explicit ownership of the
     * reference.
     */
    array_preamble *release() {
        return reinterpret_cast<array_preamble *>(m_memblock.release());
    }

    /** Low level access to the reference-counted memory */
    inline memory_block_ptr get_memblock() const {
        return m_memblock;
    }

    /** Low level access to the array preamble */
    inline array_preamble *get_ndo() const {
        return reinterpret_cast<array_preamble *>(m_memblock.get());
    }

    /** Low level access to the array metadata */
    inline const char *get_ndo_meta() const {
        return reinterpret_cast<const char *>(get_ndo() + 1);
    }

    /** Low level access to the array metadata */
    inline char *get_ndo_meta() {
        return reinterpret_cast<char *>(get_ndo() + 1);
    }

    /** Returns true if the array is NULL */
    inline bool is_empty() const {
        return m_memblock.get() == NULL;
    }

    inline char *get_readwrite_originptr() const {
        if (get_ndo()->m_flags & write_access_flag) {
            return get_ndo()->m_data_pointer;
        } else {
            throw std::runtime_error("tried to write to a dynd array that is not writeable");
        }
    }

    inline const char *get_readonly_originptr() const {
        return get_ndo()->m_data_pointer;
    }

    inline uint32_t get_access_flags() const {
        return get_ndo()->m_flags & (immutable_access_flag | read_access_flag | write_access_flag);
    }

    /** Returns true if the object is a scalar */
    inline bool is_scalar() const {
        return get_dtype().is_scalar();
    }

    /** The dtype */
    inline const dtype& get_dtype() const {
        return *reinterpret_cast<const dtype *>(&get_ndo()->m_dtype);
    }

    inline size_t get_undim() const {
        if (get_ndo()->is_builtin_dtype()) {
            return 0;
        } else {
            return get_ndo()->m_dtype->get_undim();
        }
    }

    /** The uniform dtype (Most similar to numpy ndarray.dtype property) */
    inline dtype get_udtype() const {
        if (get_ndo()->is_builtin_dtype()) {
            return dtype(get_ndo()->get_builtin_type_id());
        } else {
            size_t undim = get_ndo()->m_dtype->get_undim();
            if (undim == 0) {
                return dtype(get_ndo()->m_dtype, true);
            } else {
                return get_ndo()->m_dtype->get_dtype_at_dimension(NULL, undim);
            }
        }
    }

    /**
     * If the caller has the only reference to this array and its data,
     * makes the access flags into read-only and immutable.
     */
    void flag_as_immutable();

    /** The flags, including access permissions. */
    inline uint64_t get_flags() const {
        return get_ndo()->m_flags;
    }

    inline std::vector<intptr_t> get_shape() const {
        std::vector<intptr_t> result(get_undim());
        get_shape(&result[0]);
        return result;
    }
    inline void get_shape(intptr_t *out_shape) const {
        if (!get_ndo()->is_builtin_dtype() && get_ndo()->m_dtype->get_undim() > 0) {
            get_ndo()->m_dtype->get_shape(get_ndo()->m_dtype->get_undim(), 0, out_shape, get_ndo_meta());
        }
    }

    /**
     * Returns the size of the leading (leftmost) dimension.
     */
    inline intptr_t get_dim_size() const {
        return get_dtype().get_dim_size(get_ndo_meta(), get_ndo()->m_data_pointer);
    }

    std::vector<intptr_t> get_strides() const {
        std::vector<intptr_t> result(get_undim());
        get_strides(&result[0]);
        return result;
    }
    inline void get_strides(intptr_t *out_strides) const {
        if (!get_ndo()->is_builtin_dtype()) {
            get_ndo()->m_dtype->get_strides(0, out_strides, get_ndo_meta());
        }
    }

    inline memory_block_ptr get_data_memblock() const {
        if (get_ndo()->m_data_reference) {
            return memory_block_ptr(get_ndo()->m_data_reference, true);
        } else {
            return m_memblock;
        }
    }

    /**
     * Accesses a dynamic property of the array.
     *
     * \param property_name  The property to access.
     */
    array p(const char *property_name) const;
    /**
     * Accesses a dynamic property of the array.
     *
     * \param property_name  The property to access.
     */
    array p(const std::string& property_name) const;
    /**
     * Finds the dynamic function of the array. Throws an
     * exception if it does not exist. To call the function,
     * use ndobj.f("funcname").call(ndobj, ...). The reason
     * ndobj.f("funcname", ...) isn't used is due to a circular
     * dependency between callable and array. A resolution
     * to this will make calling these functions much more
     * convenient.
     *
     * \param function_name  The name of the function.
     */
    const gfunc::callable& find_dynamic_function(const char *function_name) const;

    /** Calls the dynamic function - #include <dynd/gfunc/call_callable.hpp> to use it */
    array f(const char *function_name);

    /** Calls the dynamic function - #include <dynd/gfunc/call_callable.hpp> to use it */
    template<class T0>
    array f(const char *function_name, const T0& p0);

    /** Calls the dynamic function - #include <dynd/gfunc/call_callable.hpp> to use it */
    template<class T0, class T1>
    array f(const char *function_name, const T0& p0, const T1& p1);

    /** Calls the dynamic function - #include <dynd/gfunc/call_callable.hpp> to use it */
    template<class T0, class T1, class T2>
    array f(const char *function_name, const T0& p0, const T1& p1, const T2& p2);

    /** Calls the dynamic function - #include <dynd/gfunc/call_callable.hpp> to use it */
    template<class T0, class T1, class T2, class T3>
    array f(const char *function_name, const T0& p0, const T1& p1, const T2& p2, const T3& p3);

    /**
     * A helper for assigning to the values in 'this'. Normal assignment to
     * an array variable has reference semantics, the reference gets
     * overwritten to point to the new array. The 'vals' function provides
     * syntactic sugar for the 'val_assign' function, allowing for more
     * natural looking assignments.
     *
     * Example:
     *      array a(make_dtype<float>());
     *      a.vals() = 100;
     */
    array_vals vals() const;

    /**
     * A helper for assigning to the values indexed in an array.
     */
    array_vals_at vals_at(const irange& i0) const;

    /**
     * A helper for assigning to the values indexed in an array.
     */
    array_vals_at vals_at(const irange& i0, const irange& i1) const;

    /**
     * A helper for assigning to the values indexed in an array.
     */
    array_vals_at vals_at(const irange& i0, const irange& i1, const irange& i2) const;

    /**
     * A helper for assigning to the values indexed in an array.
     */
    array_vals_at vals_at(const irange& i0, const irange& i1, const irange& i2, const irange& i3) const;


    /**
     * Evaluates the array, attempting to do the minimum work
     * required. If the array is not ane expression, simply
     * returns it as is, otherwise evaluates into a new copy.
     */
    array eval(const eval::eval_context *ectx = &eval::default_eval_context) const;

    /**
     * Evaluates the array into an immutable strided array, or
     * returns it untouched if it is already both immutable and strided.
     */
    array eval_immutable(const eval::eval_context *ectx = &eval::default_eval_context) const;

    /**
     * Evaluates the array node into a newly allocated strided array,
     * with the requested access flags.
     *
     * \param access_flags  The access flags for the result, default read and write.
     */
    array eval_copy(const eval::eval_context *ectx = &eval::default_eval_context,
                        uint32_t access_flags=read_access_flag|write_access_flag) const;

    /**
     * Returns a view of the array as bytes (for POD) or the storage dtype,
     * peeling away any expression dtypes or encodings.
     */
    array storage() const;

    /**
     * General irange-based indexing operation.
     *
     * \param nindices  The number of 'irange' indices.
     * \param indices  The array of indices to apply.
     * \param collapse_leading  If true, collapses the leading dimension
     *                          to simpler dtypes where possible. If false,
     *                          does not. If you want to read values, typically
     *                          use true, if you want to write values, typically
     *                          use false.
     */
    array at_array(size_t nindices, const irange *indices, bool collapse_leading = true) const;

    /**
     * The 'at' function is used for indexing. Overloading operator[] isn't
     * practical for multidimensional objects.
     */
    array at(const irange& i0) const {
        return at_array(1, &i0);
    }

    /** Indexing with two index values */
    array at(const irange& i0, const irange& i1) const {
        irange i[2] = {i0, i1};
        return at_array(2, i);
    }

    /** Indexing with three index values */
    array at(const irange& i0, const irange& i1, const irange& i2) const {
        irange i[3] = {i0, i1, i2};
        return at_array(3, i);
    }
    /** Indexing with four index values */
    array at(const irange& i0, const irange& i1, const irange& i2, const irange& i3) const {
        irange i[4] = {i0, i1, i2, i3};
        return at_array(4, i);
    }

    /** Does a value-assignment from the rhs array. */
    void val_assign(const array& rhs, assign_error_mode errmode = assign_error_default,
                        const eval::eval_context *ectx = &eval::default_eval_context) const;
    /** Does a value-assignment from the rhs raw scalar */
    void val_assign(const dtype& rhs_dt, const char *rhs_metadata, const char *rhs_data,
                        assign_error_mode errmode = assign_error_default,
                        const eval::eval_context *ectx = &eval::default_eval_context) const;

    /**
     * Casts the dtype of the array into the specified dtype.
     * This casts the entire dtype. If you want to cast the
     * uniform dtype, use 'ucast' instead.
     *
     * \param dt  The dtype into which the array should be cast.
     * \param errmode  Policy for dealing with errors.
     */
    array cast(const dtype& dt, assign_error_mode errmode = assign_error_default) const;

    /**
     * Casts the uniform dtype of the array into the specified dtype.
     *
     * \param uniform_dt  The dtype into which the array's
     *                    uniform type should be cast.
     * \param replace_undim  The number of uniform dimensions of
     *                       this dtype which should be replaced.
     *                       E.g. the value 1 could cast the last
     *                       uniform dimension and the uniform dtype
     *                       to the replacement uniform_dt.
     * \param errmode  Policy for dealing with errors.
     */
    array ucast(const dtype& uniform_dt,
                    size_t replace_undim = 0,
                    assign_error_mode errmode = assign_error_default) const;

    /**
     * Casts the uniform dtype of the array into the type specified
     * as the template parameter.
     */
    template<class T>
    inline array ucast(size_t replace_undim = 0,
                    assign_error_mode errmode = assign_error_default) const {
        return ucast(make_dtype<T>(), replace_undim, errmode);
    }

    /**
     * Replaces the uniform dtype with a new one, returning a view to
     * the result. The new dtype must have the same storage as the
     * existing dtype.
     *
     * \param new_udtype  The replacement dtype.
     * \param replace_undim  The number of uniform dimensions to replace
     *                       in addition to the uniform dtype.
     */
    array replace_udtype(const dtype& new_udtype, size_t replace_undim = 0) const;

    /**
     * Views the array's memory as another dtype, where such an operation
     * makes sense. This is analogous to reinterpret_cast<>.
     */
    array view_scalars(const dtype& scalar_dtype) const;

    /**
     * Views the array's memory as another dtype, where such an operation
     * makes sense. This is analogous to reinterpret_case<>.
     */
    template<class T>
    array view_scalars() const {
        return view_scalars(make_dtype<T>());
    }

    /**
     * When this is a zero-dimensional array, converts it to a C++ scalar of the
     * requested template type. This function may be extended in the future for
     * 1D vectors (as<std::vector<T>>), matrices, etc.
     *
     * \param errmode  The assignment error mode to use.
     */
    template<class T>
    T as(assign_error_mode errmode = assign_error_default) const;

    /** Sorting comparison between two arrays. (Returns a bool, does not broadcast) */
    bool op_sorting_less(const array& rhs) const;
    /** Less than comparison between two arrays. (Returns a bool, does not broadcast) */
    bool operator<(const array& rhs) const;
    /** Less equal comparison between two arrays. (Returns a bool, does not broadcast) */
    bool operator<=(const array& rhs) const;
    /** Equality comparison between two arrays. (Returns a bool, does not broadcast) */
    bool operator==(const array& rhs) const;
    /** Inequality comparison between two arrays. (Returns a bool, does not broadcast) */
    bool operator!=(const array& rhs) const;
    /** Greator equal comparison between two arrays. (Returns a bool, does not broadcast) */
    bool operator>=(const array& rhs) const;
    /** Greater than comparison between two arrays. (Returns a bool, does not broadcast) */
    bool operator>(const array& rhs) const;

    bool equals_exact(const array& rhs) const;

    void debug_print(std::ostream& o, const std::string& indent = "") const;

    friend std::ostream& operator<<(std::ostream& o, const array& rhs);
    friend class array_vals;
    friend class array_vals_at;
};

array operator+(const array& op0, const array& op1);
array operator-(const array& op0, const array& op1);
array operator/(const array& op0, const array& op1);
array operator*(const array& op0, const array& op1);

/**
 * This is a helper class for dealing with value assignment and collapsing
 * a view-based array into a strided array. Only the array class itself
 * is permitted to construct this helper object, and it is non-copyable.
 *
 * All that can be done is assigning the values of the referenced array
 * to another array, or assigning values from another array into the elements
 * the referenced array.
 */
class array_vals {
    const array& m_arr;
    array_vals(const array& arr)
        : m_arr(arr) {
    }

    // Non-copyable, not default-constructable
    array_vals(const array_vals&);
    array_vals& operator=(const array_vals&);
public:
    /**
     * Assigns values from an array to the internally referenced array.
     * this does a val_assign with the default assignment error mode.
     */
    array_vals& operator=(const array& rhs) {
        m_arr.val_assign(rhs);
        return *this;
    }

    /** Does a value-assignment from the rhs C++ scalar. */
    template<class T>
    typename enable_if<is_dtype_scalar<T>::value, array_vals&>::type operator=(const T& rhs) {
        m_arr.val_assign(make_dtype<T>(), NULL, (const char *)&rhs);
        return *this;
    }
    /**
     * Does a value-assignment from the rhs C++ boolean scalar.
     *
     * By default, many things are convertible to bool, and this will cause
     * screwed up assignments if we accept any such thing. Thus, we use
     * enable_if to only allow bools here instead of just accepting "const bool&"
     * as would seem obvious.
     */
    template<class T>
    typename enable_if<is_type_bool<T>::value, array_vals&>::type  operator=(const T& rhs) {
        dynd_bool v = rhs;
        m_arr.val_assign(make_dtype<dynd_bool>(), NULL, (const char *)&v);
        return *this;
    }

    // TODO: Could also do +=, -=, *=, etc.

    friend class array;
    friend array_vals array::vals() const;
};

/**
 * This is a helper class like array_vals, but it holds a reference
 * to the temporary array. This is needed by vals_at.
 *
 */
class array_vals_at {
    array m_arr;
    array_vals_at(const array& arr)
        : m_arr(arr) {
    }

#ifdef DYND_RVALUE_REFS
    array_vals_at(array&& arr)
        : m_arr(DYND_MOVE(arr)) {
    }
#endif

    // Non-copyable, not default-constructable
    array_vals_at(const array_vals&);
    array_vals_at& operator=(const array_vals_at&);
public:
    /**
     * Assigns values from an array to the internally referenced array.
     * this does a val_assign with the default assignment error mode.
     */
    array_vals_at& operator=(const array& rhs) {
        m_arr.val_assign(rhs);
        return *this;
    }

    /** Does a value-assignment from the rhs C++ scalar. */
    template<class T>
    typename enable_if<is_dtype_scalar<T>::value, array_vals&>::type operator=(const T& rhs) {
        m_arr.val_assign(make_dtype<T>(), NULL, (const char *)&rhs);
        return *this;
    }
    /**
     * Does a value-assignment from the rhs C++ boolean scalar.
     *
     * By default, many things are convertible to bool, and this will cause
     * screwed up assignments if we accept any such thing. Thus, we use
     * enable_if to only allow bools here instead of just accepting "const bool&"
     * as would seem obvious.
     */
    template<class T>
    typename enable_if<is_type_bool<T>::value, array_vals&>::type  operator=(const T& rhs) {
        dynd_bool v = rhs;
        m_arr.val_assign(make_dtype<dynd_bool>(), NULL, (const char *)&v);
        return *this;
    }

    // TODO: Could also do +=, -=, *=, etc.

    friend class array;
    friend array_vals_at array::vals_at(const irange&) const;
    friend array_vals_at array::vals_at(const irange&, const irange&) const;
    friend array_vals_at array::vals_at(const irange&, const irange&, const irange&) const;
    friend array_vals_at array::vals_at(const irange&, const irange&, const irange&, const irange&) const;
};

/** Makes a strided array with uninitialized data. If axis_perm is NULL, it is C-order */
array make_strided_array(const dtype& uniform_dtype, size_t ndim, const intptr_t *shape,
                int64_t access_flags = read_access_flag|write_access_flag, const int *axis_perm = NULL);

/**
 * \brief Makes a strided array pointing to existing data
 *
 * \param uniform_dtype  The dtype of each element in the strided array.
 * \param ndim  The number of strided dimensions.
 * \param shape  The shape of the strided dimensions.
 * \param strides  The strides of the strided dimensions.
 * \param access_flags Read/write/immutable flags.
 * \param data_ptr  Pointer to the element at index 0.
 * \param data_reference  A memory block which holds a reference to the data.
 * \param out_uniform_metadata  If the uniform_dtype has metadata (get_metadata_size() > 0),
 *                              this must be non-NULL, and is populated with a pointer to the
 *                              metadata for the uniform_dtype. The caller must populate it
 *                              with valid data.
 *
 * \returns  The created array.
 */
array make_strided_array_from_data(const dtype& uniform_dtype, size_t ndim, const intptr_t *shape,
                const intptr_t *strides, int64_t access_flags, char *data_ptr,
                const memory_block_ptr& data_reference, char **out_uniform_metadata = NULL);

/** Makes a POD (plain old data) array with data initialized by the provided pointer */
array make_pod_array(const dtype& pod_dt, const void *data);

array make_string_array(const char *str, size_t len, string_encoding_t encoding);
inline array make_ascii_array(const char *str, size_t len) {
    return make_string_array(str, len, string_encoding_ascii);
}
inline array make_utf8_array(const char *str, size_t len) {
    return make_string_array(str, len, string_encoding_utf_8);
}
inline array make_utf16_array(const uint16_t *str, size_t len) {
    return make_string_array(reinterpret_cast<const char *>(str), len * sizeof(uint16_t), string_encoding_utf_16);
}
inline array make_utf32_array(const uint32_t *str, size_t len) {
    return make_string_array(reinterpret_cast<const char *>(str), len * sizeof(uint32_t), string_encoding_utf_32);
}

template<int N>
inline array make_ascii_array(const char (&static_string)[N]) {
    return make_ascii_array(&static_string[0], N);
}
template<int N>
inline array make_utf8_array(const char (&static_string)[N]) {
    return make_utf8_array(&static_string[0], N);
}
template<int N>
inline array make_utf8_array(const unsigned char (&static_string)[N]) {
    return make_utf8_array(reinterpret_cast<const char *>(&static_string[0]), N);
}
template<int N>
inline array make_utf16_array(const uint16_t (&static_string)[N]) {
    return make_utf16_array(&static_string[0], N);
}
template<int N>
inline array make_utf32_array(const uint32_t (&static_string)[N]) {
    return make_utf32_array(&static_string[0], N);
}

/**
 * \brief Creates an array array of strings.
 *
 * \param cstr_array  An array of NULL-terminated UTF8 strings.
 * \param array_size  The number of elements in `cstr_array`.
 *
 * \returns  An array of type strided_dim<string<utf_8>>.
 */
array make_utf8_array_array(const char **cstr_array, size_t array_size);

inline array make_strided_array(intptr_t shape0, const dtype& uniform_dtype) {
    return make_strided_array(uniform_dtype, 1, &shape0, read_access_flag|write_access_flag, NULL);
}
inline array make_strided_array(intptr_t shape0, intptr_t shape1, const dtype& uniform_dtype) {
    intptr_t shape[2] = {shape0, shape1};
    return make_strided_array(uniform_dtype, 2, shape, read_access_flag|write_access_flag, NULL);
}
inline array make_strided_array(intptr_t shape0, intptr_t shape1, intptr_t shape2, const dtype& uniform_dtype) {
    intptr_t shape[3] = {shape0, shape1, shape2};
    return make_strided_array(uniform_dtype, 3, shape, read_access_flag|write_access_flag, NULL);
}

inline array_vals array::vals() const {
    return array_vals(*this);
}

inline array_vals_at array::vals_at(const irange& i0) const {
    return array_vals_at(at_array(1, &i0, false));
}

inline array_vals_at array::vals_at(const irange& i0, const irange& i1) const {
    irange i[2] = {i0, i1};
    return array_vals_at(at_array(2, i, false));
}

inline array_vals_at array::vals_at(const irange& i0, const irange& i1,
                const irange& i2) const {
    irange i[3] = {i0, i1, i2};
    return array_vals_at(at_array(3, i, false));
}

inline array_vals_at array::vals_at(const irange& i0, const irange& i1,
                const irange& i2, const irange& i3) const {
    irange i[4] = {i0, i1, i2, i3};
    return array_vals_at(at_array(4, i, false));
}

///////////// Initializer list constructor implementation /////////////////////////
#ifdef DYND_INIT_LIST
namespace detail {
    // Computes the number of dimensions in a nested initializer list constructor
    template<class T>
    struct initializer_list_ndim {static const int value = 0;};
    template<class T>
    struct initializer_list_ndim<std::initializer_list<T> > {
        static const int value = initializer_list_ndim<T>::value + 1;
    };

    // Computes the array dtype of a nested initializer list constructor
    template<class T>
    struct initializer_list_dtype {typedef T type;};
    template<class T>
    struct initializer_list_dtype<std::initializer_list<T> > {
        typedef typename initializer_list_dtype<T>::type type;
    };

    // Gets the shape of the nested initializer list constructor, and validates that
    // it isn't ragged
    template <class T>
    struct initializer_list_shape;
    // Base case, an initializer list parameterized by a non-initializer list
    template<class T>
    struct initializer_list_shape<std::initializer_list<T> > {
        static void compute(intptr_t *out_shape, const std::initializer_list<T>& il) {
            out_shape[0] = il.size();
        }
        static void validate(const intptr_t *shape, const std::initializer_list<T>& il) {
            if ((intptr_t)il.size() != shape[0]) {
                throw std::runtime_error("initializer list for array is ragged, must be "
                                        "nested in a regular fashion");
            }
        }
        static void copy_data(T **dataptr, const std::initializer_list<T>& il) {
            DYND_MEMCPY(*dataptr, il.begin(), il.size() * sizeof(T));
            *dataptr += il.size();
        }
    };
    // Recursive case, an initializer list parameterized by an initializer list
    template<class T>
    struct initializer_list_shape<std::initializer_list<std::initializer_list<T> > > {
        static void compute(intptr_t *out_shape,
                        const std::initializer_list<std::initializer_list<T> >& il) {
            out_shape[0] = il.size();
            if (out_shape[0] > 0) {
                // Recursively compute the rest of the shape
                initializer_list_shape<std::initializer_list<T> >::
                                                compute(out_shape + 1, *il.begin());
                // Validate the shape for the nested initializer lists
                for (auto i = il.begin() + 1; i != il.end(); ++i) {
                    initializer_list_shape<std::initializer_list<T> >::
                                                        validate(out_shape + 1, *i);
                }
            }
        }
        static void validate(const intptr_t *shape,
                        const std::initializer_list<std::initializer_list<T> >& il) {
            if ((intptr_t)il.size() != shape[0]) {
                throw std::runtime_error("initializer list for array is ragged, must be "
                                        "nested in a regular fashion");
            }
            // Validate the shape for the nested initializer lists
            for (auto i = il.begin(); i != il.end(); ++i) {
                initializer_list_shape<std::initializer_list<T> >::validate(shape + 1, *i);
            }
        }
        static void copy_data(typename initializer_list_dtype<T>::type **dataptr,
                        const std::initializer_list<std::initializer_list<T> >& il) {
            for (auto i = il.begin(); i != il.end(); ++i) {
                initializer_list_shape<std::initializer_list<T> >::copy_data(dataptr, *i);
            }
        }
    };
} // namespace detail

// Implementation of initializer list construction
template<class T>
dynd::array::array(std::initializer_list<T> il)
    : m_memblock()
{
    intptr_t dim0 = il.size();
    intptr_t stride = (dim0 == 1) ? 0 : sizeof(T);
    char *originptr = 0;
    memory_block_ptr memblock = make_fixed_size_pod_memory_block(sizeof(T) * dim0, sizeof(T), &originptr);
    DYND_MEMCPY(originptr, il.begin(), sizeof(T) * dim0);
    make_strided_array_node(make_dtype<T>(), 1, &dim0, &stride,
                            originptr, read_access_flag | write_access_flag, DYND_MOVE(memblock)).swap(m_memblock);
}
template<class T>
dynd::array::array(std::initializer_list<std::initializer_list<T> > il)
    : m_memblock()
{
    typedef std::initializer_list<std::initializer_list<T> > S;
    intptr_t shape[2], strides[2];

    // Get and validate that the shape is regular
    detail::initializer_list_shape<S>::compute(shape, il);
    // Compute the number of elements in the array, and the strides at the same time
    intptr_t num_elements = 1, stride = sizeof(T);
    for (int i = 1; i >= 0; --i) {
        strides[i] = (shape[i] == 1) ? 0 : stride;
        num_elements *= shape[i];
        stride *= shape[i];
    }
    char *originptr = 0;
    memory_block_ptr memblock = make_fixed_size_pod_memory_block(sizeof(T) * num_elements, sizeof(T), &originptr);
    T *dataptr = reinterpret_cast<T *>(originptr);
    detail::initializer_list_shape<S>::copy_data(&dataptr, il);
    make_strided_array_node(make_dtype<T>(), 2, shape, strides,
                        originptr, read_access_flag | write_access_flag, DYND_MOVE(memblock)).swap(m_memblock);
}
template<class T>
dynd::array::array(std::initializer_list<std::initializer_list<std::initializer_list<T> > > il)
    : m_memblock()
{
    typedef std::initializer_list<std::initializer_list<std::initializer_list<T> > > S;
    intptr_t shape[3], strides[3];

    // Get and validate that the shape is regular
    detail::initializer_list_shape<S>::compute(shape, il);
    // Compute the number of elements in the array, and the strides at the same time
    intptr_t num_elements = 1, stride = sizeof(T);
    for (int i = 2; i >= 0; --i) {
        strides[i] = (shape[i] == 1) ? 0 : stride;
        num_elements *= shape[i];
        stride *= shape[i];
    }
    char *originptr = 0;
    memory_block_ptr memblock = make_fixed_size_pod_memory_block(sizeof(T) * num_elements, sizeof(T), &originptr);
    T *dataptr = reinterpret_cast<T *>(originptr);
    detail::initializer_list_shape<S>::copy_data(&dataptr, il);
    make_strided_array_node(make_dtype<T>(), 3, shape, strides,
                    originptr, read_access_flag | write_access_flag, DYND_MOVE(memblock)).swap(m_memblock);
}
#endif // DYND_INIT_LIST

///////////// C-style array constructor implementation /////////////////////////
namespace detail {
    template<class T> struct uniform_type_from_array {
        typedef T type;
        static const size_t element_size = sizeof(T);
        static const int type_id = type_id_of<T>::value;
    };
    template<class T, int N> struct uniform_type_from_array<T[N]> {
        typedef typename uniform_type_from_array<T>::type type;
        static const size_t element_size = uniform_type_from_array<T>::element_size;
        static const int type_id = uniform_type_from_array<T>::type_id;
    };

    template<class T> struct ndim_from_array {static const int value = 0;};
    template<class T, int N> struct ndim_from_array<T[N]> {
        static const int value = ndim_from_array<T>::value + 1;
    };

    template<class T> struct fill_shape {
        static size_t fill(intptr_t *) {
            return sizeof(T);
        }
    };
    template<class T, int N> struct fill_shape<T[N]> {
        static size_t fill(intptr_t *out_shape) {
            out_shape[0] = N;
            return N * fill_shape<T>::fill(out_shape + 1);
        }
    };
} // namespace detail

template<class T, int N>
nd::array::array(const T (&rhs)[N])
    : m_memblock()
{
    const int ndim = detail::ndim_from_array<T[N]>::value;
    intptr_t shape[ndim];
    size_t size = detail::fill_shape<T[N]>::fill(shape);

    *this = make_strided_array(dtype(static_cast<type_id_t>(detail::uniform_type_from_array<T>::type_id)),
                    ndim, shape, read_access_flag|write_access_flag, NULL);
    DYND_MEMCPY(get_ndo()->m_data_pointer, reinterpret_cast<const void *>(&rhs), size);
}

template<int N>
inline nd::array::array(const char (&rhs)[N])
{
    make_string_array(rhs, N, string_encoding_utf_8).swap(*this);
}

template<int N>
inline nd::array::array(const char *(&rhs)[N])
{
    make_utf8_array_array(rhs, N).swap(*this);
}

///////////// std::vector constructor implementation /////////////////////////
namespace detail {
    template <class T>
    struct make_from_vec {
        inline static typename enable_if<is_dtype_scalar<T>::value, array>::type
                        make(const std::vector<T>& vec)
        {
            array result = make_strided_array(vec.size(), make_dtype<T>());
            if (!vec.empty()) {
                memcpy(result.get_readwrite_originptr(), &vec[0], vec.size() * sizeof(T));
            }
            return result;
        }
    };

    template <>
    struct make_from_vec<dtype> {
        static array make(const std::vector<dtype>& vec);
    };

    template <>
    struct make_from_vec<std::string> {
        static array make(const std::vector<std::string>& vec);
    };
} // namespace detail

template<class T>
array::array(const std::vector<T>& vec)
{
   detail::make_from_vec<T>::make(vec).swap(*this);
}


///////////// The array.as<type>() templated function /////////////////////////
namespace detail {
    template <class T>
    struct array_as_helper {
        static typename enable_if<is_dtype_scalar<T>::value, T>::type as(const array& lhs,
                                                                    assign_error_mode errmode) {
            T result;
            if (!lhs.is_scalar()) {
                throw std::runtime_error("can only convert arrays with 0 dimensions to scalars");
            }
            dtype_assign(make_dtype<T>(), NULL, (char *)&result,
                        lhs.get_dtype(), lhs.get_ndo_meta(), lhs.get_ndo()->m_data_pointer, errmode);
            return result;
        }
    };

    template <>
    struct array_as_helper<bool> {
        static bool as(const array& lhs, assign_error_mode errmode) {
            return array_as_helper<dynd_bool>::as(lhs, errmode);
        }
    };

    std::string array_as_string(const array& lhs, assign_error_mode errmode);
    dtype array_as_dtype(const array& lhs, assign_error_mode errmode);

    template <>
    struct array_as_helper<std::string> {
        static std::string as(const array& lhs, assign_error_mode errmode) {
            return array_as_string(lhs, errmode);
        }
    };

    template <>
    struct array_as_helper<dtype> {
        static dtype as(const array& lhs, assign_error_mode errmode) {
            return array_as_dtype(lhs, errmode);
        }
    };

    // Could do as<std::vector<T>> for 1D arrays, and other similiar conversions
} // namespace detail;

template<class T>
T array::as(assign_error_mode errmode) const {
    return detail::array_as_helper<T>::as(*this, errmode);
}

/** 
 * Given the dtype/metadata/data of an array (or sub-component of an array),
 * evaluates a new copy of it as the canonical dtype.
 */
array eval_raw_copy(const dtype& dt, const char *metadata, const char *data);

/**
 * Constructs an uninitialized array of the given dtype.
 */
array empty(const dtype& dt);

/**
 * Constructs an uninitialized array of the given dtype,
 * specified as a string. This is a shortcut for expressions
 * like
 *
 *      array a = nd::empty("10, int32");
 */
template<int N>
inline array empty(const char (&dshape)[N]) {
    return empty(dtype(dshape, dshape + N - 1));
}

/**
 * Constructs a writeable uninitialized array of the specified dtype.
 * This dtype should be at least one dimensional, and is initialized
 * using the specified dimension size.
 */
array empty(intptr_t dim0, const dtype& dt);

/**
 * Constructs an uninitialized array of the given dtype,
 * specified as a string. This is a shortcut for expressions
 * like
 *
 *      array a = nd::empty(10, "M, int32");
 */
template<int N>
inline array empty(intptr_t dim0, const char (&dshape)[N]) {
    return empty(dim0, dtype(dshape, dshape + N - 1));
}

/**
 * Constructs a writeable uninitialized array of the specified dtype.
 * This dtype should be at least two dimensional, and is initialized
 * using the specified dimension sizes.
 */
array empty(intptr_t dim0, intptr_t dim1, const dtype& dt);

/**
 * Constructs an uninitialized array of the given dtype,
 * specified as a string. This is a shortcut for expressions
 * like
 *
 *      array a = nd::empty(10, 10, "M, N, int32");
 */
template<int N>
inline array empty(intptr_t dim0, intptr_t dim1, const char (&dshape)[N]) {
    return empty(dim0, dim1, dtype(dshape, dshape + N - 1));
}

/**
 * Constructs a writeable uninitialized array of the specified dtype.
 * This dtype should be at least three dimensional, and is initialized
 * using the specified dimension sizes.
 */
array empty(intptr_t dim0, intptr_t dim1, intptr_t dim2, const dtype& dt);

/**
 * Constructs an uninitialized array of the given dtype,
 * specified as a string. This is a shortcut for expressions
 * like
 *
 *      array a = nd::empty(10, 10, 10, "M, N, R, int32");
 */
template<int N>
inline array empty(intptr_t dim0, intptr_t dim1, intptr_t dim2, const char (&dshape)[N]) {
    return empty(dim0, dim1, dim2, dtype(dshape, dshape + N - 1));
}

/**
 * Constructs an array with the same shape and memory layout
 * of the one given, but replacing the
 *
 * \param rhs  The array whose shape and memory layout to emulate.
 * \param uniform_dtype   The uniform dtype of the new array.
 */
array empty_like(const array& rhs, const dtype& uniform_dtype);

/**
 * Constructs an empty array matching the parameters of 'rhs'
 *
 * \param rhs  The array whose shape, memory layout, and dtype to emulate.
 */
array empty_like(const array& rhs);

/**
 * Performs a binary search of the first dimension of the array, which
 * should be sorted. The data/metadata must correspond to the dtype n.get_dtype().at(0).
 *
 * \returns  The index of the found element, or -1 if not found.
 */
intptr_t binary_search(const array& n, const char *data, const char *metadata);

array groupby(const array& data_values, const array& by,
                const dtype& groups = dtype());

/**
 * Creates a cstruct array with the given field names and
 * pointers to the provided field values.
 *
 * \param  field_count  The number of fields.
 * \param  field_names  The names of the fields.
 * \param  field_values  The values of the fields.
 */
array combine_into_struct(size_t field_count, const std::string *field_names,
                    const array *field_values);

}} // namespace dynd::nd

#endif // _DYND__NDOBJECT_HPP_