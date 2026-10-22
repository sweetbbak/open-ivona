/*
 * CIvonaTTS (Thumb C++ TU at the start of .text): the public tts_* API as a table of function
 * pointers. Clients dlopen() the engine and call v() to get it.
 */
#include "ivona_tts.h"

class CIvonaTTS {
public:
    CIvonaTTS();

    __typeof__(&tts_vendor_tag) vendor_tag;
    __typeof__(&tts_errno) errno_;
    __typeof__(&tts_errmsg) errmsg;
    __typeof__(&tts_init) init;
    __typeof__(&tts_create) create;
    __typeof__(&tts_load_certificate_file) load_certificate_file;
    __typeof__(&tts_load_certificate) load_certificate;
    __typeof__(&tts_destroy) destroy;
    __typeof__(&tts_certificate_get) certificate_get;
    __typeof__(&tts_certificate_get_single) certificate_get_single;
    __typeof__(&tts_load_voice) load_voice;
    __typeof__(&tts_unload_voice) unload_voice;
    __typeof__(&tts_voice_param) voice_param;
    __typeof__(&tts_voice_sample_rate) voice_sample_rate;
    __typeof__(&tts_dict_insert) dict_insert;
    __typeof__(&tts_dict_remove) dict_remove;
    __typeof__(&tts_dict_save) dict_save;
    __typeof__(&tts_dict_load) dict_load;
    __typeof__(&tts_dict_num_entries) dict_num_entries;
    __typeof__(&tts_dict_cleanup) dict_cleanup;
    __typeof__(&tts_pls_load) pls_load;
    __typeof__(&tts_pls_unload) pls_unload;
    __typeof__(&tts_streamer_start) streamer_start;
    __typeof__(&tts_streamer_stop) streamer_stop;
    __typeof__(&tts_streamer_jump) streamer_jump;
    __typeof__(&tts_streamer_add_voice) streamer_add_voice;
    __typeof__(&tts_streamer_synth0) streamer_synth0;
    __typeof__(&tts_streamer_synth) streamer_synth;
    __typeof__(&tts_wave_delete) wave_delete;
    __typeof__(&tts_get_input_warning) get_input_warning;
};

/* 0x18e58 thumb */
CIvonaTTS::CIvonaTTS()
{
    vendor_tag = tts_vendor_tag;
    errno_ = tts_errno;
    errmsg = tts_errmsg;
    init = tts_init;
    create = tts_create;
    load_certificate_file = tts_load_certificate_file;
    load_certificate = tts_load_certificate;
    destroy = tts_destroy;
    certificate_get = tts_certificate_get;
    certificate_get_single = tts_certificate_get_single;
    load_voice = tts_load_voice;
    unload_voice = tts_unload_voice;
    voice_param = tts_voice_param;
    voice_sample_rate = tts_voice_sample_rate;
    dict_insert = tts_dict_insert;
    dict_remove = tts_dict_remove;
    dict_save = tts_dict_save;
    dict_load = tts_dict_load;
    dict_num_entries = tts_dict_num_entries;
    dict_cleanup = tts_dict_cleanup;
    pls_load = tts_pls_load;
    pls_unload = tts_pls_unload;
    streamer_start = tts_streamer_start;
    streamer_stop = tts_streamer_stop;
    streamer_jump = tts_streamer_jump;
    streamer_add_voice = tts_streamer_add_voice;
    streamer_synth0 = tts_streamer_synth0;
    streamer_synth = tts_streamer_synth;
    wave_delete = tts_wave_delete;
    get_input_warning = tts_get_input_warning;
}

/* 0x18f94 thumb: the singleton API table. */
extern "C" CIvonaTTS *v(void)
{
    static CIvonaTTS api;
    return &api;
}
