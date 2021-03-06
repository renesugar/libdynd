//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/types/scalar_kind_type.hpp>

namespace dynd {
namespace ndt {

  class DYNDT_API int_kind_type : public base_type {
  public:
    int_kind_type(type_id_t id) : base_type(id, 0, 1, type_flag_symbolic, 0, 0, 0) {}

    bool match(const type &candidate_tp, std::map<std::string, type> &tp_vars) const;

    void print_data(std::ostream &o, const char *arrmeta, const char *data) const;

    void print_type(std::ostream &o) const;

    bool operator==(const base_type &rhs) const;
  };

  template <>
  struct id_of<int_kind_type> : std::integral_constant<type_id_t, int_kind_id> {};

} // namespace dynd::ndt
} // namespace dynd
