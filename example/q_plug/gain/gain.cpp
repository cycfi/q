#include <clap/clap.h>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstdio>

// --------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------
static constexpr clap_id PARAM_VOLUME = 0;
static constexpr double VOLUME_MIN    = 0.0;
static constexpr double VOLUME_MAX    = 2.0;
static constexpr double VOLUME_DEF    = 1.0;

// --------------------------------------------------------------------------
// Plugin data
// --------------------------------------------------------------------------
struct QPlugGain {
    const clap_host_t* host;
    double             volume; // current gain
};

// --------------------------------------------------------------------------
// clap_plugin_audio_ports
// --------------------------------------------------------------------------
static uint32_t audio_ports_count(const clap_plugin_t*, bool) { return 1; }

static bool audio_ports_get(const clap_plugin_t*, uint32_t index,
                             bool is_input, clap_audio_port_info_t* info) {
    if (index != 0) return false;
    info->id            = 0;
    info->channel_count = 2;
    info->flags         = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type     = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    snprintf(info->name, sizeof(info->name), is_input ? "Input" : "Output");
    return true;
}

static const clap_plugin_audio_ports_t s_audio_ports = {
    audio_ports_count,
    audio_ports_get,
};

// --------------------------------------------------------------------------
// clap_plugin_params
// --------------------------------------------------------------------------
static uint32_t params_count(const clap_plugin_t*) { return 1; }

static bool params_get_info(const clap_plugin_t*, uint32_t index,
                             clap_param_info_t* info) {
    if (index != 0) return false;
    info->id            = PARAM_VOLUME;
    info->flags         = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
    info->min_value     = VOLUME_MIN;
    info->max_value     = VOLUME_MAX;
    info->default_value = VOLUME_DEF;
    info->cookie        = nullptr;
    snprintf(info->name,   sizeof(info->name),   "Volume");
    snprintf(info->module, sizeof(info->module), "");
    return true;
}

static bool params_get_value(const clap_plugin_t* plugin, clap_id param_id,
                              double* value) {
    if (param_id != PARAM_VOLUME) return false;
    auto* self = static_cast<QPlugGain*>(plugin->plugin_data);
    *value = self->volume;
    return true;
}

static bool params_value_to_text(const clap_plugin_t*, clap_id param_id,
                                  double value, char* display, uint32_t size) {
    if (param_id != PARAM_VOLUME) return false;
    snprintf(display, size, "%.3f", value);
    return true;
}

static bool params_text_to_value(const clap_plugin_t*, clap_id param_id,
                                  const char* display, double* value) {
    if (param_id != PARAM_VOLUME) return false;
    *value = atof(display);
    return true;
}

static void params_flush(const clap_plugin_t* plugin,
                          const clap_input_events_t*  in,
                          const clap_output_events_t* /*out*/) {
    auto* self = static_cast<QPlugGain*>(plugin->plugin_data);
    uint32_t n = in->size(in);
    for (uint32_t i = 0; i < n; ++i) {
        const clap_event_header_t* hdr = in->get(in, i);
        if (hdr->type == CLAP_EVENT_PARAM_VALUE &&
            hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
            auto* ev = reinterpret_cast<const clap_event_param_value_t*>(hdr);
            if (ev->param_id == PARAM_VOLUME)
                self->volume = ev->value;
        }
    }
}

static const clap_plugin_params_t s_params = {
    params_count,
    params_get_info,
    params_get_value,
    params_value_to_text,
    params_text_to_value,
    params_flush,
};

// --------------------------------------------------------------------------
// clap_plugin_state
// --------------------------------------------------------------------------
static bool state_save(const clap_plugin_t* plugin, const clap_ostream_t* stream) {
    auto* self = static_cast<QPlugGain*>(plugin->plugin_data);
    int64_t written = stream->write(stream, &self->volume, sizeof(self->volume));
    return written == (int64_t)sizeof(self->volume);
}

static bool state_load(const clap_plugin_t* plugin, const clap_istream_t* stream) {
    auto* self = static_cast<QPlugGain*>(plugin->plugin_data);
    double v   = VOLUME_DEF;
    int64_t n  = stream->read(stream, &v, sizeof(v));
    if (n == (int64_t)sizeof(v)) {
        self->volume = v;
        return true;
    }
    return false;
}

static const clap_plugin_state_t s_state = { state_save, state_load };

// --------------------------------------------------------------------------
// clap_plugin lifecycle
// --------------------------------------------------------------------------
static bool plugin_init(const clap_plugin_t* plugin) {
    auto* self   = static_cast<QPlugGain*>(plugin->plugin_data);
    self->volume = VOLUME_DEF;
    return true;
}

static void plugin_destroy(const clap_plugin_t* plugin) {
    delete static_cast<QPlugGain*>(plugin->plugin_data);
}

static bool plugin_activate(const clap_plugin_t*, double, uint32_t, uint32_t) {
    return true;
}
static void plugin_deactivate(const clap_plugin_t*) {}
static bool plugin_start_processing(const clap_plugin_t*) { return true; }
static void plugin_stop_processing(const clap_plugin_t*) {}
static void plugin_reset(const clap_plugin_t*) {}

static clap_process_status plugin_process(const clap_plugin_t* plugin,
                                          const clap_process_t* process) {
    auto* self = static_cast<QPlugGain*>(plugin->plugin_data);

    // Handle param events
    if (process->in_events) {
        uint32_t n = process->in_events->size(process->in_events);
        for (uint32_t i = 0; i < n; ++i) {
            const clap_event_header_t* hdr =
                process->in_events->get(process->in_events, i);
            if (hdr->type == CLAP_EVENT_PARAM_VALUE &&
                hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
                auto* ev =
                    reinterpret_cast<const clap_event_param_value_t*>(hdr);
                if (ev->param_id == PARAM_VOLUME)
                    self->volume = ev->value;
            }
        }
    }

    uint32_t frames = process->frames_count;
    float    gain   = (float)self->volume;

    for (uint32_t ch = 0; ch < 2; ++ch) {
        const float* in  = process->audio_inputs[0].data32[ch];
        float*       out = process->audio_outputs[0].data32[ch];
        for (uint32_t f = 0; f < frames; ++f)
            out[f] = in[f] * gain;
    }

    return CLAP_PROCESS_CONTINUE;
}

static const void* plugin_get_extension(const clap_plugin_t*, const char* id) {
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &s_audio_ports;
    if (!strcmp(id, CLAP_EXT_PARAMS))      return &s_params;
    if (!strcmp(id, CLAP_EXT_STATE))       return &s_state;
    return nullptr;
}

static void plugin_on_main_thread(const clap_plugin_t*) {}

// --------------------------------------------------------------------------
// Factory
// --------------------------------------------------------------------------
static const char* s_features[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr,
};

static const clap_plugin_descriptor_t s_desc = {
    CLAP_VERSION,
    "com.qplug.gain",
    "QPlug Gain",
    "QPlug",
    "",      // url
    "",      // manual_url
    "",      // support_url
    "0.1.0",
    "Simple stereo gain plugin",
    s_features,
};

static const clap_plugin_t* factory_create(const clap_plugin_factory_t*,
                                            const clap_host_t*  host,
                                            const char*         plugin_id) {
    if (strcmp(plugin_id, s_desc.id) != 0) return nullptr;

    auto* self     = new QPlugGain();
    self->host     = host;
    self->volume   = VOLUME_DEF;

    auto* plugin   = new clap_plugin_t();
    plugin->desc         = &s_desc;
    plugin->plugin_data  = self;
    plugin->init         = plugin_init;
    plugin->destroy      = plugin_destroy;
    plugin->activate     = plugin_activate;
    plugin->deactivate   = plugin_deactivate;
    plugin->start_processing = plugin_start_processing;
    plugin->stop_processing  = plugin_stop_processing;
    plugin->reset        = plugin_reset;
    plugin->process      = plugin_process;
    plugin->get_extension = plugin_get_extension;
    plugin->on_main_thread = plugin_on_main_thread;

    return plugin;
}

static uint32_t factory_count(const clap_plugin_factory_t*) { return 1; }

static const clap_plugin_descriptor_t* factory_get_desc(
    const clap_plugin_factory_t*, uint32_t index) {
    return index == 0 ? &s_desc : nullptr;
}

static const clap_plugin_factory_t s_factory = {
    factory_count,
    factory_get_desc,
    factory_create,
};

// --------------------------------------------------------------------------
// Entry functions — exported for the entry shim and the clap_entry below.
// These are named with the qplug_ prefix so the entry shim can link them
// without colliding with the clap_entry symbol exported by each format.
// --------------------------------------------------------------------------
extern "C" bool qplug_entry_init(const char* /*plugin_path*/) { return true; }
extern "C" void qplug_entry_deinit() {}
extern "C" const void* qplug_entry_get_factory(const char* factory_id) {
    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID)) return &s_factory;
    return nullptr;
}
