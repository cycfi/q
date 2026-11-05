/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "bender_controller.hpp"

using parameter_list = bender_controller::parameter_list;

parameter_list bender_controller::parameters() const
{
   static parameter params[] =
   {
      parameter{ 1, "Dive", 12.0 }.range(0.0, 12.0).unit("st"),
      parameter{ 2, "Pull-up", 12.0 }.range(0.0, 12.0).unit("st"),
      parameter{ 3, "Middle", 1.0 }.range(0.0, 12.0).unit("st"),
      parameter{ 4, "Formants", true },
      parameter{ 5, "Formant Amount", 100.0 }.range(0.0, 100.0).unit("%"),
      parameter{ 6, "Freeze", false }
   };

   return { params };
}

// The learned table, row by row: each row's reflection coefficients, then
// its level.
void bender_controller::save_extra(json& j) const
{
   auto t = worker.saved_table();
   if (!t)
      return;

   auto rows = json::array();
   for (std::size_t r = 0; r != t->num_rows; ++r)
   {
      auto row = json::array();
      for (auto k : t->k(r))
         row.push_back(k);
      row.push_back(t->level(r));
      rows.push_back(std::move(row));
   }
   j["formants"] = {{"order", t->order()}, {"rows", std::move(rows)}};
}

void bender_controller::load_extra(json const& j, std::uint32_t /*version*/)
{
   auto f = j.find("formants");
   if (f == j.end())
      return;

   auto const order = f->value("order", std::size_t{0});
   auto const& rows = f->at("rows");
   if (order == 0 || rows.size() != q::formant_table::num_rows)
      return;

   q::formant_table t{order};
   for (std::size_t r = 0; r != rows.size(); ++r)
   {
      auto const& row = rows[r];
      if (row.size() != order + 1)
         return;
      auto k = t.k(r);
      for (std::size_t i = 0; i != order; ++i)
         k[i] = row[i].get<float>();
      t.level(r) = row[order].get<float>();
   }
   worker.restore(t);
}
