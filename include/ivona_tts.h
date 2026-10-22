#ifndef _IVONA_TTS_H_
#define _IVONA_TTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IVONA_TTS_VERSION 0x00010635

const char* tts_vendor_tag();

enum tts_status {
    TTS_STATUS_OK,
    TTS_STATUS_INVALID_CERTIFICATE,
    TTS_STATUS_CERTIFICATE_EXPIRED,
    TTS_STATUS_CERTIFICATE_LIMIT,
    TTS_STATUS_LOAD_VOICE_ERROR,
    TTS_STATUS_FORMAT_ERROR,
    TTS_STATUS_RANGE_ERROR,
    TTS_STATUS_USER_ABORTED,
    TTS_STATUS_INVALID_ARGUMENT,
    TTS_STATUS_SYSTEM_ERROR,
    TTS_STATUS_INTERNAL_ERROR,
    TTS_STATUS_LOAD_LANGUGE_ERROR
};

enum tts_status tts_errno(void);

const char* tts_errmsg(void);

typedef struct tts_instance_struct tts_instance;

typedef struct tts_certificate_struct tts_certificate;

tts_instance* tts_init(const char* certificate_text);

tts_instance* tts_create(void);

const tts_certificate*
tts_load_certificate_file(tts_instance* tts, const char* cert_path);

const tts_certificate*
tts_load_certificate(tts_instance* tts, const char* cert_text);

int tts_destroy(tts_instance* tts);

const char* tts_certificate_get(const tts_instance* tts, const char* attr);

const char*
tts_certificate_get_single(const tts_certificate* cert, const char* attr);

typedef struct tts_voice_lib_struct {
    const char* name;
    const char* language;
    const char* lang_codes;
    const char* gender;
    const char* lector;
    const char* version;
    const void* vox_internals;
    const void* vox_data;
    const char* lector_ascii;
} tts_voice_lib;

typedef struct tts_dict_entry_struct {
    char* rule;
    char* change_with;
    void* rule_data;
    void* extra;
    char* dict_id;
} tts_dict_entry;

typedef struct tts_voice_struct {
    tts_instance* tts;
    const tts_voice_lib* voice;
    const char* vox_name;
    int sample_rate;
    tts_dict_entry** dict;
    void* voice_feats;
    void* voice_specific;
} tts_voice;

tts_voice*
tts_load_voice(tts_instance* tts, const char* lib_path, const char* vox_path);

int tts_unload_voice(tts_voice* v);

int tts_voice_param(tts_voice* v, const char* command, void* io_val);

int tts_voice_sample_rate(const tts_voice* v);

int tts_dict_insert(
    tts_voice* v,
    const char* rule,
    const char* change_with,
    int index
);

int tts_dict_remove(tts_voice* v, int index);

int tts_dict_save(tts_voice* v, const char* filename, const char* dict_id);

int tts_dict_load(tts_voice* v, const char* filename);

int tts_dict_num_entries(const tts_voice* v);

void tts_dict_cleanup(tts_voice* v);

int tts_pls_load(tts_voice* v, const char* filename);

int tts_pls_unload(tts_voice* v, int id);

typedef struct tts_streamer_struct {
    tts_voice* voice_ptr;
    void* data;
} tts_streamer;

tts_streamer* tts_streamer_start(tts_voice* v, const char* t, float speed);

int tts_streamer_stop(tts_streamer* st);

int tts_streamer_jump(tts_streamer* st, int length);

int tts_streamer_add_voice(tts_streamer* st, tts_voice* v);

#define TTS_ALL_VOICES ((tts_voice*)-1)

int tts_streamer_synth0(tts_streamer* st, short* samples, int num_samples);

enum tts_mark_type {
    TTS_MARK_PHONEME,
    TTS_MARK_WORD,
    TTS_MARK_SENTENCE,
    TTS_MARK_SSML,
    TTS_MARK_VISEME,
    TTS_MARK_VOICE
};

typedef struct tts_mark_struct {
    enum tts_mark_type type;
    const char* name;
    unsigned int start_text_offset;
    unsigned int end_text_offset;
    unsigned int sample_offset;
} tts_mark;

typedef struct tts_wave_struct {
    int sample_rate;
    unsigned int num_samples;
    short* samples;
    unsigned int num_marks;
    tts_mark* marks;
    void* data;
} tts_wave;

tts_wave* tts_streamer_synth(tts_streamer* st, int num_samples);

void tts_wave_delete(tts_wave* w);

enum tts_input_warning_code {
    TTS_INPUT_WARNING_NONE,
    TTS_INPUT_WARNING_UNSUPPORTED_ALPHABET,
    TTS_INPUT_WARNING_UNSUPPORTED_LANG,
    TTS_INPUT_WARNING_UNSUPPORTED_LEXICON,
    TTS_INPUT_WARNING_UNSUPPORTED_VERSION,
    TTS_INPUT_WARNING_ATTR_MISSING,
    TTS_INPUT_WARNING_ATTR_INVALID,
    TTS_INPUT_WARNING_SSML_STRUCTURE,
    TTS_INPUT_WARNING_XML,
    TTS_INPUT_WARNING_INVALID_ENTITY_EXPANSION,
    TTS_INPUT_WARNING_FETCH_FAILED,
    TTS_INPUT_WARNING_INVALID_URI,
    TTS_INPUT_WARNING_LOOKUP_IGNORED,
    TTS_INPUT_WARNING_UNSUPPORTED_AUDIO,
    TTS_INPUT_WARNING_UNSUPPORTED_VOICE
};

enum tts_input_warning_code tts_get_input_warning(
    tts_streamer* st,
    int* byte_offset,
    int* line,
    const char** msg
);

typedef struct tts_fetch_request_struct {
    void* user_data;
    int maxage;
    int maxstale;
    float timeout;
    const char* original_uri;
    const char* base_uri;
    const char* absolute_uri;
} tts_fetch_request;

typedef struct tts_fetch_result_struct {
    void* ptr;
    unsigned int length;
    char* mime_type;
    void (*free_func)(struct tts_fetch_result_struct* res);
    void* free_func_data;
} tts_fetch_result;

enum tts_fetch_code {
    TTS_FETCH_FAILED,
    TTS_FETCH_ABORT,
    TTS_FETCH_USE_DEFAULT,
    TTS_FETCH_SUCCEEDED
};

typedef enum tts_fetch_code (*tts_fetch_func)(
    const tts_fetch_request* req,
    tts_fetch_result* res
);

typedef struct {
    const char* const src;
    const char* const absolute_uri;
    const char* const base_uri;
    int clip_begin_set;
    float clip_begin;
    int clip_end_set;
    float clip_end;
    int sound_level_set;
    float sound_level;
    int speed_set;
    float speed;
    int repeat_count_set;
    float repeat_count;
    int repeat_dur_set;
    float repeat_dur;
} tts_audio_attributes;

typedef void (*tts_audio_attributes_func)(
    tts_audio_attributes* attrs,
    void* data
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _IVONA_TTS_H_ */
