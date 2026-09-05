/*
 * qplug_entry.cpp
 *
 * Thin shim that wires the impl library's entry functions to the
 * CLAP export symbol. This file is re-compiled once per plugin format
 * by clap-wrapper (CLAP, VST3, AUv2) so each format binary gets its own
 * properly-named export.
 */

#include <clap/clap.h>

extern "C" {
    bool        qplug_entry_init(const char* plugin_path);
    void        qplug_entry_deinit(void);
    const void* qplug_entry_get_factory(const char* factory_id);
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    qplug_entry_init,
    qplug_entry_deinit,
    qplug_entry_get_factory,
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
