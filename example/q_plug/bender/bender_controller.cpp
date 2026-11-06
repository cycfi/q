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
      parameter{ 6, "Freeze", false },
      parameter{ 7, "Bend", 0.0 }.range(-1.0, 1.0).live().dont_save()
   };

   return { params };
}

// What the learner has learned, so a session comes back with it and
// learning goes on: the average spectrum, its frame count, the sample
// rate it was learned at, and the cells seen.
void bender_controller::save_extra(json& j) const
{
   // Nothing learned is nothing to save.
   auto s = worker.saved();
   if (!s || !(s->frames > 0))
      return;

   j["formants"] = {
      {"sps", s->sps}
    , {"frames", s->frames}
    , {"spectrum", s->spectrum}
    , {"seen", s->seen}
   };
}

// A state that does not parse is left out: the learner starts afresh.
void bender_controller::load_extra(json const& j, std::uint32_t /*version*/)
{
   auto f = j.find("formants");
   if (f == j.end() || !f->is_object())
      return;

   try
   {
      formant_worker::state_type s;
      s.sps = f->at("sps").get<float>();
      s.frames = f->at("frames").get<double>();
      s.spectrum = f->at("spectrum").get<std::vector<float>>();
      s.seen = f->at("seen").get<std::vector<bool>>();
      worker.restore(std::move(s));
   }
   catch (json::exception const&)
   {
   }
}
