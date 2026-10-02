/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PROFILES_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_PROFILES_HPP_SEPTEMBER_18_2026

#include <q/midi/ci.hpp>

namespace cycfi::q::midi_ci
{
   ////////////////////////////////////////////////////////////////////////////
   // MIDI-CI Profile Configuration, category 2. M2-101-UM version 1.2
   // section 7 for the messages, and M2-102-U Common Rules for MIDI-CI
   // Profiles version 1.1 for what they mean.
   //
   // A profile is an agreement about what a device does with the messages
   // it already understands: a drawbar organ profile says which controller
   // is which drawbar. A device says which profiles it has and which are
   // on, and the other end turns them on and off.
   //
   // What a device declares is a list, and the responder replies from it.
   // Turning one on is the one thing that changes, and 2.6 asks that the
   // reply state what actually happened rather than whether it worked, so
   // a profile that cannot be turned off meets a Set Profile Off with an
   // Enabled report.
   ////////////////////////////////////////////////////////////////////////////
   namespace profile_status
   {
      enum
      {
         inquiry           = 0x20,
         inquiry_reply     = 0x21,
         set_on            = 0x22,
         set_off           = 0x23,
         enabled_report    = 0x24,
         disabled_report   = 0x25,
         added_report      = 0x26,
         removed_report    = 0x27,
         details           = 0x28,
         details_reply     = 0x29,
         specific_data     = 0x2F
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // 7.3.1, Table 19: five bytes, in the order they are sent and not as a
   // number. A first byte of 0x7E is a profile the Association defined;
   // anything else is a manufacturer's own system exclusive id, with a one
   // byte id followed by two zeros.
   ////////////////////////////////////////////////////////////////////////////
   struct profile_id
   {
      std::uint8_t   byte1 = 0;
      std::uint8_t   byte2 = 0;
      std::uint8_t   byte3 = 0;
      std::uint8_t   byte4 = 0;     // version, or the manufacturer's own
      std::uint8_t   byte5 = 0;     // level, or the manufacturer's own

      constexpr bool standard() const  { return byte1 == universal; }
   };

   constexpr bool operator==(profile_id const& a, profile_id const& b)
   {
      return a.byte1 == b.byte1 && a.byte2 == b.byte2
         && a.byte3 == b.byte3 && a.byte4 == b.byte4 && a.byte5 == b.byte5;
   }

   // 3.2, Table 13: what byte 5 says about how much of a profile is there.
   namespace profile_level
   {
      enum
      {
         partial  = 0x00,
         minimum  = 0x01,
         // 3.2: what a sender asks for, meaning all a receiver has.
         highest  = 0x7F
      };
   }

   // Two profiles are the same profile when they differ only in the level
   // byte, which 3.2 has a sender set to 0x7F when asking for one.
   constexpr bool same_profile(profile_id const& a, profile_id const& b)
   {
      return a.byte1 == b.byte1 && a.byte2 == b.byte2
         && a.byte3 == b.byte3 && a.byte4 == b.byte4;
   }

   ////////////////////////////////////////////////////////////////////////////
   // What a device declares about one profile it has. 2.3: a profile
   // belongs to a channel, to a group, or to the whole function block, and
   // the address is where its messages are sent.
   //
   // A permanent profile is one whose state cannot be changed, which 2.5
   // gives as an acoustic piano that always conforms to the piano profile.
   // Asking to turn one on or off is met with the state it is in.
   ////////////////////////////////////////////////////////////////////////////
   struct profile
   {
      profile_id     id;
      std::uint8_t   address = to_function_block;
      bool           enabled = false;
      bool           permanent = false;

      // 2.5.1: how many channels it is using now, and the most it could.
      // Zero for a profile on a group or a function block.
      std::uint16_t  channels = 0;
      std::uint16_t  max_channels = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Views
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      constexpr profile_id read_profile(byte_span b)
      {
         return {b[0], b[1], b[2], b[3], b[4]};
      }

      constexpr std::size_t write_profile(
         std::uint8_t* out, profile_id const& id)
      {
         out[0] = id.byte1 & 0x7F;
         out[1] = id.byte2 & 0x7F;
         out[2] = id.byte3 & 0x7F;
         out[3] = id.byte4 & 0x7F;
         out[4] = id.byte5 & 0x7F;
         return 5;
      }
   }

   // 7.3, Table 18: the profiles that are on, then the ones that are off.
   struct profile_inquiry_reply_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == profile_status::inquiry_reply
            && _b.size() >= header_size + 2
            && _b.size() >= header_size + 4 + 5*(enabled() + disabled());
      }

      constexpr std::uint16_t    enabled() const
                                 { return read14(_b.subspan(13)); }

      constexpr std::uint16_t    disabled() const
                                 {
                                    return read14(
                                       _b.subspan(15 + 5*enabled()));
                                 }

      constexpr profile_id       enabled_profile(std::size_t i) const
                                 { return detail::read_profile(
                                      _b.subspan(15 + 5*i)); }

      constexpr profile_id       disabled_profile(std::size_t i) const
                                 { return detail::read_profile(
                                      _b.subspan(17 + 5*enabled() + 5*i)); }
   };

   // 7.8, 7.9, Tables 24 and 25. The two byte field after the id is the
   // channels asked for on a Set Profile On and is reserved on a Set
   // Profile Off, and version 1.2 added both.
   struct set_profile_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header()
            && (sub_id() == profile_status::set_on
               || sub_id() == profile_status::set_off)
            && _b.size() >= header_size + 5;
      }

      constexpr bool             on() const
                                 { return sub_id() == profile_status::set_on; }
      constexpr profile_id       id() const
                                 { return detail::read_profile(
                                      _b.subspan(13)); }

      // Zero when a version 1.1 sender left it out.
      constexpr std::uint16_t    channels() const
                                 {
                                    return _b.size() >= header_size + 7
                                       ? read14(_b.subspan(18)) : 0;
                                 }
   };

   // 7.10, 7.11, Tables 26 and 27.
   struct profile_report_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header()
            && (sub_id() == profile_status::enabled_report
               || sub_id() == profile_status::disabled_report)
            && _b.size() >= header_size + 5;
      }

      constexpr bool             enabled() const
                                 {
                                    return sub_id()
                                       == profile_status::enabled_report;
                                 }
      constexpr profile_id       id() const
                                 { return detail::read_profile(
                                      _b.subspan(13)); }
      constexpr std::uint16_t    channels() const
                                 {
                                    return _b.size() >= header_size + 7
                                       ? read14(_b.subspan(18)) : 0;
                                 }
   };

   // 7.4, 7.5, Tables 20 and 21: a profile a device has gained or lost.
   struct profile_list_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header()
            && (sub_id() == profile_status::added_report
               || sub_id() == profile_status::removed_report)
            && _b.size() >= header_size + 5;
      }

      constexpr bool             added() const
                                 {
                                    return sub_id()
                                       == profile_status::added_report;
                                 }
      constexpr profile_id       id() const
                                 { return detail::read_profile(
                                      _b.subspan(13)); }
   };

   // 7.6, Table 22. Targets below 0x40 mean the same thing for every
   // profile; 0x40 and above are the profile's own.
   namespace profile_target
   {
      enum
      {
         channels = 0x00      // the only one the Common Rules define
      };
   }

   struct profile_details_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == profile_status::details
            && _b.size() >= header_size + 6;
      }

      constexpr profile_id       id() const
                                 { return detail::read_profile(
                                      _b.subspan(13)); }
      constexpr std::uint8_t     target() const    { return _b[18]; }
   };

   // 7.7, Table 23, and Common Rules Table 8 for target 0x00, whose data
   // is the channels in use now and the most that could be.
   struct profile_details_reply_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == profile_status::details_reply
            && _b.size() >= header_size + 8
            && _b.size() >= header_size + 8 + length();
      }

      constexpr profile_id       id() const
                                 { return detail::read_profile(
                                      _b.subspan(13)); }
      constexpr std::uint8_t     target() const    { return _b[18]; }
      constexpr std::uint16_t    length() const
                                 { return read14(_b.subspan(19)); }
      constexpr byte_span        data() const
                                 { return _b.subspan(21, length()); }

      // Target 0x00 only.
      constexpr std::uint16_t    channels() const
                                 { return read14(_b.subspan(21)); }
      constexpr std::uint16_t    max_channels() const
                                 { return read14(_b.subspan(23)); }
   };

   // 7.12, Table 28. Its length is four bytes where every other one in
   // this category is two, and MIDI-CI defines no chunking for it.
   struct profile_data_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == profile_status::specific_data
            && _b.size() >= header_size + 9;
      }

      constexpr profile_id       id() const
                                 { return detail::read_profile(
                                      _b.subspan(13)); }
      constexpr std::uint32_t    length() const
                                 { return read_muid(_b.subspan(18)); }
      constexpr byte_span        data() const
                                 { return _b.subspan(22, length()); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Builders
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::size_t make_profile_inquiry(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t address = to_function_block)
   {
      auto n = detail::header_at(
         out, address, profile_status::inquiry, source, destination);
      out[n++] = 0xF7;
      return n;
   }

   // 7.3: the enabled profiles and their count, then the disabled ones.
   template <typename Profiles>
   constexpr std::size_t make_profile_inquiry_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t address, Profiles const& profiles)
   {
      auto n = detail::header_at(
         out, address, profile_status::inquiry_reply, source, destination);

      for (int pass = 0; pass != 2; ++pass)
      {
         std::uint16_t count = 0;
         for (auto const& p : profiles)
            if (p.address == address && p.enabled == (pass == 0))
               ++count;

         write14(out+n, count);
         n += 2;

         for (auto const& p : profiles)
            if (p.address == address && p.enabled == (pass == 0))
               n += detail::write_profile(out+n, p.id);
      }

      out[n++] = 0xF7;
      return n;
   }

   // 7.8, 7.9. The channels field is what a Set Profile On asks for, and
   // is reserved and zero on a Set Profile Off.
   constexpr std::size_t make_set_profile(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t address, profile_id const& id, bool on
    , std::uint16_t channels = 0)
   {
      auto n = detail::header_at(
         out, address
       , on? profile_status::set_on : profile_status::set_off
       , source, destination);
      n += detail::write_profile(out+n, id);
      write14(out+n, on? channels : 0);
      n += 2;
      out[n++] = 0xF7;
      return n;
   }

   // 7.10, 7.11: both go to everyone, and say the state that now holds.
   constexpr std::size_t make_profile_report(
      std::uint8_t* out, std::uint32_t source, std::uint8_t address
    , profile_id const& id, bool enabled, std::uint16_t channels = 0)
   {
      auto n = detail::header_at(
         out, address
       , enabled? profile_status::enabled_report
          : profile_status::disabled_report
       , source, broadcast_muid);
      n += detail::write_profile(out+n, id);
      write14(out+n, channels);
      n += 2;
      out[n++] = 0xF7;
      return n;
   }

   // 7.4, 7.5: a profile a device has gained or lost, also to everyone.
   constexpr std::size_t make_profile_list_report(
      std::uint8_t* out, std::uint32_t source, std::uint8_t address
    , profile_id const& id, bool added)
   {
      auto n = detail::header_at(
         out, address
       , added? profile_status::added_report
          : profile_status::removed_report
       , source, broadcast_muid);
      n += detail::write_profile(out+n, id);
      out[n++] = 0xF7;
      return n;
   }

   // 7.6.
   constexpr std::size_t make_profile_details(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t address, profile_id const& id
    , std::uint8_t target = profile_target::channels)
   {
      auto n = detail::header_at(
         out, address, profile_status::details, source, destination);
      n += detail::write_profile(out+n, id);
      out[n++] = target & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 7.7 with Common Rules Table 8: the channels in use and the most
   // there could be, which is what target 0x00 asks for.
   constexpr std::size_t make_profile_details_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t address, profile_id const& id
    , std::uint16_t channels, std::uint16_t max_channels)
   {
      auto n = detail::header_at(
         out, address, profile_status::details_reply, source, destination);
      n += detail::write_profile(out+n, id);
      out[n++] = profile_target::channels;
      write14(out+n, 4);
      n += 2;
      write14(out+n, channels);
      n += 2;
      write14(out+n, max_channels);
      n += 2;
      out[n++] = 0xF7;
      return n;
   }

   ////////////////////////////////////////////////////////////////////////////
   // profile_responder: the device's side of profile configuration. It
   // replies from the list it was given and passes everything else to the
   // responder behind it.
   //
   //    ci::responder ci{identity, rng};
   //    ci::profile_responder profiles{my_profiles, ci};
   //    profiles(sysex_view, send);
   //
   // The list is the caller's, and this writes through to it: turning a
   // profile on is the one thing a device is asked to do here, so the
   // application sees it as a changed flag rather than as a message.
   //
   // 2.4: an inquiry addressed to the function block gets one reply per
   // address that has profiles, channels first, then the group, and the
   // one addressed to the function block last, which is what tells the
   // asker no more are coming.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Next, std::size_t Capacity = default_max_sysex_size>
   class profile_responder
   {
   public:

                              profile_responder(
                                 std::span<profile> profiles, Next next)
                               : _profiles(profiles)
                               , _next(std::forward<Next>(next))
                              {}

                              template <typename Send>
      void                    operator()(
                                 midi_1_0::sysex_view msg, Send&& send);

      std::span<profile>      profiles() const  { return _profiles; }

      // The discovery responder's, forwarded so that responders nest: a
      // device with profiles and properties builds one chain of them.
      std::uint32_t           muid() const      { return _next.muid(); }

                              template <typename Send>
      void                    announce(Send&& send) { _next.announce(send); }

   private:

                              template <typename Send>
      void                    reply_to_inquiry(
                                 std::uint8_t address, std::uint32_t to
                               , Send& send);
                              template <typename Send>
      void                    send_at(std::size_t n, Send& send)
                              {
                                 midi::detail::emit(send, byte_span{
                                    _out.data(), n});
                              }

      profile*                find(profile_id const& id, std::uint8_t at);

      std::span<profile>      _profiles;
      Next                    _next;
      std::array<std::uint8_t, Capacity> _out = {};
   };

   template <typename Next>
   profile_responder(std::span<profile>, Next&&) -> profile_responder<Next>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Next, std::size_t Capacity>
   inline profile* profile_responder<Next, Capacity>::find(
      profile_id const& id, std::uint8_t at)
   {
      for (auto& p : _profiles)
         if (p.address == at && same_profile(p.id, id))
            return &p;
      return nullptr;
   }

   template <typename Next, std::size_t Capacity>
   template <typename Send>
   inline void profile_responder<Next, Capacity>::reply_to_inquiry(
      std::uint8_t address, std::uint32_t to, Send& send)
   {
      send_at(
         make_profile_inquiry_reply(
            _out.data(), _next.muid(), to, address, _profiles)
       , send);
   }

   template <typename Next, std::size_t Capacity>
   template <typename Send>
   inline void profile_responder<Next, Capacity>::operator()(
      midi_1_0::sysex_view msg, Send&& send)
   {
      message_view const m{msg.data()};
      auto const mine = m.has_header() && m.destination() == _next.muid()
         && m.sub_id() >= profile_status::inquiry
         && m.sub_id() <= profile_status::specific_data;

      if (!mine)
      {
         _next(msg, send);
         return;
      }

      auto const at = m.device_id();
      switch (m.sub_id())
      {
         case profile_status::inquiry:
         {
            // 2.4: one reply per address when asked at the function block,
            // and the function block's own last.
            if (at != to_function_block)
            {
               reply_to_inquiry(at, m.source(), send);
               return;
            }

            for (std::uint8_t ch = 0; ch != 16; ++ch)
               for (auto const& p : _profiles)
                  if (p.address == ch)
                  {
                     reply_to_inquiry(ch, m.source(), send);
                     break;
                  }

            for (auto const& p : _profiles)
               if (p.address == to_group)
               {
                  reply_to_inquiry(to_group, m.source(), send);
                  break;
               }

            reply_to_inquiry(to_function_block, m.source(), send);
            return;
         }

         case profile_status::set_on:
         case profile_status::set_off:
         {
            set_profile_view const v{msg.data()};
            if (!v.valid())
               return;

            auto* p = find(v.id(), at);
            if (p == nullptr)
            {
               // 2.6, 2.7: a profile we do not have, on this address.
               send_at(
                  make_nak(
                     _out.data(), _next.muid(), m.source(), m.sub_id()
                   , nak_status::profile_not_supported)
                , send);
               return;
            }

            // 2.6, 2.7: the reply states what now holds, and a permanent
            // profile holds what it held.
            if (!p->permanent)
               p->enabled = v.on();

            send_at(
               make_profile_report(
                  _out.data(), _next.muid(), at, p->id, p->enabled
                , p->enabled? p->channels : 0)
             , send);
            return;
         }

         case profile_status::details:
         {
            profile_details_view const v{msg.data()};
            if (!v.valid())
               return;

            auto* p = find(v.id(), at);
            if (p == nullptr || v.target() != profile_target::channels)
            {
               send_at(
                  make_nak(
                     _out.data(), _next.muid(), m.source(), m.sub_id()
                   , nak_status::profile_not_supported)
                , send);
               return;
            }

            // 2.5.1: zero channels in use while the profile is off.
            send_at(
               make_profile_details_reply(
                  _out.data(), _next.muid(), m.source(), at, p->id
                , p->enabled? p->channels : 0, p->max_channels)
             , send);
            return;
         }

         default:
            // The reports and the profile specific data are for the
            // application, and the reserved ones are nobody's.
            return;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // profile_initiator: the asking side of profile configuration.
   //
   //    midi_ci::profile_initiator profiles{my_chain};
   //    profiles.ask(device, send);              // what does it have?
   //    profiles.turn_on(device, address, id, send);
   //    profiles(sysex_view, send);              // the answers arrive here
   //
   // It keeps no table: what a device has is the application's to keep.
   // Next hears each profile a reply or report names through
   // profile_state(muid, address, id, enabled), and profiles_listed(muid)
   // when the function block's reply to an inquiry arrives, which 7.3
   // sends last. It sits in front of the responder chain and shares its
   // MUID, and passes on everything that is not a reply or report for us.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Next>
   class profile_initiator
   {
   public:

      explicit                profile_initiator(Next next)
                               : _next(std::forward<Next>(next))
                              {}

                              template <typename Send>
      void                    ask(remote_device const& d, Send&& send);

                              template <typename Send>
      void                    turn_on(
                                 remote_device const& d
                               , std::uint8_t address, profile_id const& id
                               , Send&& send, std::uint16_t channels = 0);

                              template <typename Send>
      void                    turn_off(
                                 remote_device const& d
                               , std::uint8_t address, profile_id const& id
                               , Send&& send);

                              template <typename Send>
      void                    operator()(
                                 midi_1_0::sysex_view msg, Send&& send);

      std::uint32_t           muid() const { return _next.muid(); }

                              template <typename Send>
      void                    announce(Send&& send) { _next.announce(send); }

      // The other initiators' hooks, passed on so that one stage at
      // the end of a chain of initiators hears them all.
                              template <typename... A>
                              requires requires (Next& n, A&&... a)
                                 { n.device_added(std::forward<A>(a)...); }
      void                    device_added(A&&... a)
                              { _next.device_added(std::forward<A>(a)...); }
                              template <typename... A>
                              requires requires (Next& n, A&&... a)
                                 { n.device_removed(std::forward<A>(a)...); }
      void                    device_removed(A&&... a)
                              { _next.device_removed(std::forward<A>(a)...); }
                              template <typename... A>
                              requires requires (Next& n, A&&... a)
                                 { n.property_reply(std::forward<A>(a)...); }
      void                    property_reply(A&&... a)
                              { _next.property_reply(std::forward<A>(a)...); }
                              template <typename... A>
                              requires requires (Next& n, A&&... a)
                                 { n.property_update(std::forward<A>(a)...); }
      void                    property_update(A&&... a)
                              { _next.property_update(std::forward<A>(a)...); }
                              template <typename... A>
                              requires requires (Next& n, A&&... a)
                                 { n.property_capabilities(std::forward<A>(a)...); }
      void                    property_capabilities(A&&... a)
                              { _next.property_capabilities(std::forward<A>(a)...); }

   private:

      void                    state(
                                 std::uint32_t muid, std::uint8_t address
                               , profile_id const& id, bool enabled);

      Next                    _next;
      std::array<std::uint8_t, max_message> _out = {};
   };

   template <typename Next>
   profile_initiator(Next&&) -> profile_initiator<Next>;

   template <typename Next>
   template <typename Send>
   inline void profile_initiator<Next>::ask(
      remote_device const& d, Send&& send)
   {
      auto const n = make_profile_inquiry(
         _out.data(), _next.muid(), d.muid, to_function_block);
      midi::detail::emit(send, byte_span{_out.data(), n});
   }

   template <typename Next>
   template <typename Send>
   inline void profile_initiator<Next>::turn_on(
      remote_device const& d, std::uint8_t address, profile_id const& id
    , Send&& send, std::uint16_t channels)
   {
      auto const n = make_set_profile(
         _out.data(), _next.muid(), d.muid, address, id, true, channels);
      midi::detail::emit(send, byte_span{_out.data(), n});
   }

   template <typename Next>
   template <typename Send>
   inline void profile_initiator<Next>::turn_off(
      remote_device const& d, std::uint8_t address, profile_id const& id
    , Send&& send)
   {
      auto const n = make_set_profile(
         _out.data(), _next.muid(), d.muid, address, id, false);
      midi::detail::emit(send, byte_span{_out.data(), n});
   }

   template <typename Next>
   inline void profile_initiator<Next>::state(
      std::uint32_t muid, std::uint8_t address, profile_id const& id
    , bool enabled)
   {
      if constexpr (
         requires { _next.profile_state(muid, address, id, true); })
      {
         _next.profile_state(muid, address, id, enabled);
      }
   }

   template <typename Next>
   template <typename Send>
   inline void profile_initiator<Next>::operator()(
      midi_1_0::sysex_view msg, Send&& send)
   {
      message_view const m{msg.data()};
      if (m.has_header() && m.source() != _next.muid())
      {
         switch (m.sub_id())
         {
            case profile_status::inquiry_reply:
            {
               profile_inquiry_reply_view const r{msg.data()};
               if (!r.valid() || r.destination() != _next.muid())
                  break;

               auto const at = r.device_id();
               for (std::size_t i = 0; i != r.enabled(); ++i)
                  state(r.source(), at, r.enabled_profile(i), true);
               for (std::size_t i = 0; i != r.disabled(); ++i)
                  state(r.source(), at, r.disabled_profile(i), false);

               if (at == to_function_block)
               {
                  if constexpr (requires { _next.profiles_listed(0u); })
                     _next.profiles_listed(r.source());
               }
               return;
            }

            // 7.10, 7.11: these go to everyone.
            case profile_status::enabled_report:
            case profile_status::disabled_report:
            {
               profile_report_view const r{msg.data()};
               if (r.valid())
                  state(r.source(), r.device_id(), r.id(), r.enabled());
               break;
            }

            default:
               break;
         }
      }
      _next(msg, send);
   }

}

#endif
