//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <bitset>

#include <dynd/type.hpp>

namespace dynd {

/**
 * Function prototype for a function which parses the type constructor arguments. It is called on the range of bytes
 * starting from the '[' character.
 */
typedef ndt::type (*low_level_type_args_parse_fn_t)(type_id_t id, const char *&begin, const char *end,
                                                    std::map<std::string, ndt::type> &symtable);
/**
 * Function prototype for a type constructor. The provided argument is a tuple containing a tuple of positional
 * arguments and a struct of keyword arguments, as is produced by `dynd::parse_type_constr_args`.
 */
typedef ndt::type (*type_constructor_fn_t)(type_id_t id, const nd::buffer &args);

/**
 * Default mechanism for parsing type arguments, when an optimized version is not implemented. It parses the type
 * arguments via `dynd::parse_type_constr_args`, then calls the type constructor for the specified type id.
 */
DYNDT_API ndt::type default_parse_type_args(type_id_t id, const char *&begin, const char *end,
                                            std::map<std::string, ndt::type> &symtable);

struct id_info {
  /** The name to use for parsing as a singleton or constructed type */
  std::string name;
  type_id_t base_id;
  /**
   * The type for parsing with no type constructor.
   *
   * If this is has uninitialized_type_id, i.e. is ndt::type(), then the type requires a type constructor
   */
  ndt::type singleton_type;
  /**
   * Low-level optimized type arguments parser. Must be equivalent to `dynd::parse_type_constr_args` followed by calling
   * the type constructor. May be set to `dynd::default_parse_type_args`.
   *
   * If this is NULL, then the type cannot be created with a type constructor and requires a singleton type.
   */
  low_level_type_args_parse_fn_t parse_type_args;
  /**
   * High-level generic construction from an nd::buffer of type arguments. Either both `parse_type_args` and
   * `construct_type` must be provided, or both must be NULL.
   *
   * If this is NULL, then the type cannot be created with a type constructor and requires a singleton type.
   */
  type_constructor_fn_t construct_type;

  id_info() = default;
  id_info(const id_info &) = default;
  id_info(id_info &&) = default;

  id_info(const char *name, type_id_t base_id) : name(name), base_id(base_id) {}
};

namespace detail {

  extern DYNDT_API std::vector<id_info> &infos();

} // namespace dynd::detail

DYNDT_API type_id_t new_id(const char *name, type_id_t base_id);

} // namespace dynd
