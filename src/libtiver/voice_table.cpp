/*
 * libtiver: registry of the IVONA voices this build knows about.
 *
 * The engine dlopen()s libtiver.so and calls v() to get a small descriptor, whose
 * get_voice callback enumerates VoiceData records by index until it returns NULL.
 */
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

/* One registered voice (0x38 bytes). */
struct VoiceData {
    const char *language;      /* ISO 639-2, e.g. "eng" */
    const char *country;       /* ISO 3166 alpha-3, e.g. "GBR" */
    const char *name;          /* "Amy" */
    const char *display_name;  /* "IVONA Amy" */
    const char *package;       /* Android package of the voice */
    const char *library;       /* voice .so loaded by the engine */
    int unk_18;                /* always -1 */
    int unk_1c;                /* always -1 */
    const char *vox_file;      /* unit database file name */
    unsigned int unk_24;       /* 122..136; h() compares the vox file's size with it */
    const char *lexicon;       /* default user lexicon file */
    const char *unk_2c;        /* always "" */
    const char *gender;        /* "Female" / "Male" */
    const char *locale;        /* BCP-47 locale, e.g. "en-GB" */
};

/* Descriptor returned by v(). */
struct VoiceRegistry {
    int struct_size;                       /* sizeof(VoiceRegistry) == 0x14 */
    int version;                           /* 2 */
    const char *kind;                      /* "sdk" */
    int entry_size;                        /* sizeof(VoiceData) == 0x38 */
    const VoiceData *(*get_voice)(int index);
};

static const VoiceData g_voice_table[] = {
    { "eng", "IND", "Aditi", "IVONA Aditi", "com.ivona.tts.oem.voicebeta.eng.ind.aditi", "libvoice_en_in_aditi.so", -1, -1, "vox_en_in_aditi22i", 130, "english.lex", "", "Female", "en-IN" }, /* 0 */
    { "eng", "GBR", "Abbey", "IVONA Abbey", "com.ivona.tts.oem.voicebeta.eng.gbr.abbey", "libvoice_en_gb_abbey.so", -1, -1, "vox_en_gb_abbey22i", 130, "english.lex", "", "Female", "en-GB" }, /* 1 */
    { "deu", "DEU", "Silke", "IVONA Silke", "com.ivona.tts.oem.voicebeta.deu.deu.silke", "libvoice_de_silke.so", -1, -1, "vox_de_silke22i", 124, "german.lex", "", "Female", "de-DE" }, /* 2 */
    { "tur", "TUR", "Filiz", "IVONA Filiz", "com.ivona.tts.oem.voicebeta.tur.tur.filiz", "libvoice_tr_filiz.so", -1, -1, "vox_tr_filiz22i", 124, "turkish.lex", "", "Female", "tr-TR" }, /* 3 */
    { "swe", "SWE", "Astrid", "IVONA Astrid", "com.ivona.tts.oem.voicebeta.swe.swe.astrid", "libvoice_sv_se_astrid.so", -1, -1, "vox_sv_se_astrid22i", 132, "swedish.lex", "", "Female", "sv-SE" }, /* 4 */
    { "rus", "RUS", "Maxim", "IVONA Maxim", "com.ivona.tts.oem.voicebeta.rus.rus.maxim", "libvoice_ru_maxim.so", -1, -1, "vox_ru_maxim22i", 124, "russian.lex", "", "Male", "ru-RU" }, /* 5 */
    { "rus", "RUS", "Tatyana", "IVONA Tatyana", "com.ivona.tts.oem.voicebeta.rus.rus.tatyana", "libvoice_ru_tatyana.so", -1, -1, "vox_ru_tatyana22i", 128, "russian.lex", "", "Female", "ru-RU" }, /* 6 */
    { "ron", "ROU", "Carmen", "IVONA Carmen", "com.ivona.tts.oem.voicebeta.ron.rou.carmen", "libvoice_ro_carmen.so", -1, -1, "vox_ro_carmen22i", 126, "romanian.lex", "", "Female", "ro-RO" }, /* 7 */
    { "por", "PRT", "Ines", "IVONA In\303\252s", "com.ivona.tts.oem.voicebeta.por.prt.ines", "libvoice_pt_pt_ines.so", -1, -1, "vox_pt_pt_ines22i", 128, "portuguese.lex", "", "Female", "pt-PT" }, /* 8 */
    { "por", "PRT", "Cristiano", "IVONA Cristiano", "com.ivona.tts.oem.voicebeta.por.prt.cristiano", "libvoice_pt_pt_cristiano.so", -1, -1, "vox_pt_pt_cristiano22i", 138, "portuguese.lex", "", "Male", "pt-PT" }, /* 9 */
    { "por", "BRA", "Vitoria", "IVONA Vit\303\263ria", "com.ivona.tts.oem.voicebeta.por.bra.vitoria", "libvoice_pt_br_vitoria.so", -1, -1, "vox_pt_br_vitoria22i", 134, "portuguese.lex", "", "Female", "pt-BR" }, /* 10 */
    { "por", "BRA", "Ricardo", "IVONA Ricardo", "com.ivona.tts.oem.voicebeta.por.bra.ricardo", "libvoice_pt_br_ricardo.so", -1, -1, "vox_pt_br_ricardo22i", 134, "portuguese.lex", "", "Male", "pt-BR" }, /* 11 */
    { "pol", "POL", "Maja", "IVONA Maja", "com.ivona.tts.oem.voicebeta.pol.pol.maja", "libvoice_pl_maja.so", -1, -1, "vox_pl_maja22i", 122, "polish.lex", "", "Female", "pl-PL" }, /* 12 */
    { "pol", "POL", "Jan", "IVONA Jan", "com.ivona.tts.oem.voicebeta.pol.pol.jan", "libvoice_pl_jan.so", -1, -1, "vox_pl_jan22i", 120, "polish.lex", "", "Male", "pl-PL" }, /* 13 */
    { "pol", "POL", "Ewa", "IVONA Ewa", "com.ivona.tts.oem.voicebeta.pol.pol.ewa", "libvoice_pl_ewa.so", -1, -1, "vox_pl_ewa22i", 120, "polish.lex", "", "Female", "pl-PL" }, /* 14 */
    { "pol", "POL", "Agnieszka", "IVONA Agnieszka", "com.ivona.tts.oem.voicebeta.pol.pol.agnieszka", "libvoice_pl_agnieszka.so", -1, -1, "vox_pl_agnieszka22i", 132, "polish.lex", "", "Female", "pl-PL" }, /* 15 */
    { "nld", "NLD", "Ruben", "IVONA Ruben", "com.ivona.tts.oem.voicebeta.nld.nld.ruben", "libvoice_nl_ruben.so", -1, -1, "vox_nl_ruben22i", 124, "dutch.lex", "", "Male", "nl-NL" }, /* 16 */
    { "nld", "NLD", "Lotte", "IVONA Lotte", "com.ivona.tts.oem.voicebeta.nld.nld.lotte", "libvoice_nl_lotte.so", -1, -1, "vox_nl_lotte22i", 124, "dutch.lex", "", "Female", "nl-NL" }, /* 17 */
    { "nob", "NOR", "Liv", "IVONA Liv", "com.ivona.tts.oem.voicebeta.nob.nor.liv", "libvoice_nb_no_liv.so", -1, -1, "vox_nb_no_liv22i", 126, "norwegian.lex", "", "Female", "nb-NO" }, /* 18 */
    { "ita", "ITA", "Giorgio", "IVONA Giorgio", "com.ivona.tts.oem.voicebeta.ita.ita.giorgio", "libvoice_it_giorgio.so", -1, -1, "vox_it_giorgio22i", 128, "italian.lex", "", "Male", "it-IT" }, /* 19 */
    { "ita", "ITA", "Carla", "IVONA Carla", "com.ivona.tts.oem.voicebeta.ita.ita.carla", "libvoice_it_carla.so", -1, -1, "vox_it_carla22i", 124, "italian.lex", "", "Female", "it-IT" }, /* 20 */
    { "isl", "ISL", "Karl", "IVONA Karl", "com.ivona.tts.oem.voicebeta.isl.isl.karl", "libvoice_is_karl.so", -1, -1, "vox_is_karl22i", 122, "icelandic.lex", "", "Male", "is-IS" }, /* 21 */
    { "isl", "ISL", "Dora", "IVONA D\303\263ra", "com.ivona.tts.oem.voicebeta.isl.isl.dora", "libvoice_is_dora.so", -1, -1, "vox_is_dora22i", 122, "icelandic.lex", "", "Female", "is-IS" }, /* 22 */
    { "fra", "FRA", "Mathieu", "IVONA Mathieu", "com.ivona.tts.oem.voicebeta.fra.fra.mathieu", "libvoice_fr_mathieu.so", -1, -1, "vox_fr_mathieu22i", 128, "french.lex", "", "Male", "fr-FR" }, /* 23 */
    { "fra", "FRA", "Celine", "IVONA C\303\251line", "com.ivona.tts.oem.voicebeta.fra.fra.celine", "libvoice_fr_celine.so", -1, -1, "vox_fr_celine22i", 126, "french.lex", "", "Female", "fr-FR" }, /* 24 */
    { "fra", "CAN", "Chantal", "IVONA Chantal", "com.ivona.tts.oem.voicebeta.fra.can.chantal", "libvoice_fr_ca_chantal.so", -1, -1, "vox_fr_ca_chantal22i", 134, "french.lex", "", "Female", "fr-CA" }, /* 25 */
    { "spa", "USA", "Penelope", "IVONA Pen\303\251lope", "com.ivona.tts.oem.voicebeta.spa.usa.penelope", "libvoice_es_us_penelope.so", -1, -1, "vox_es_us_penelope22i", 136, "spanish.lex", "", "Female", "es-US" }, /* 26 */
    { "spa", "USA", "Miguel", "IVONA Miguel", "com.ivona.tts.oem.voicebeta.spa.usa.miguel", "libvoice_es_us_miguel.so", -1, -1, "vox_es_us_miguel22i", 132, "spanish.lex", "", "Male", "es-US" }, /* 27 */
    { "spa", "ESP", "Enrique", "IVONA Enrique", "com.ivona.tts.oem.voicebeta.spa.esp.enrique", "libvoice_es_enrique.so", -1, -1, "vox_es_enrique22i", 128, "spanish.lex", "", "Male", "es-ES" }, /* 28 */
    { "spa", "ESP", "Conchita", "IVONA Conchita", "com.ivona.tts.oem.voicebeta.spa.esp.conchita", "libvoice_es_conchita.so", -1, -1, "vox_es_conchita22i", 130, "spanish.lex", "", "Female", "es-ES" }, /* 29 */
    { "eng", "GBR", "Gwyneth", "IVONA Gwyneth", "com.ivona.tts.oem.voicebeta.eng.gbr.gwyneth", "libvoice_en_wls_gwyneth.so", -1, -1, "vox_en_wls_gwyneth22i", 136, "english.lex", "", "Female", "en-GB-WLS" }, /* 30 */
    { "eng", "GBR", "Geraint", "IVONA Geraint", "com.ivona.tts.oem.voicebeta.eng.gbr.geraint", "libvoice_en_wls_geraint.so", -1, -1, "vox_en_wls_geraint22i", 136, "english.lex", "", "Male", "en-GB-WLS" }, /* 31 */
    { "eng", "USA", "Salli", "IVONA Salli", "com.ivona.tts.oem.voicebeta.eng.usa.salli", "libvoice_en_us_salli.so", -1, -1, "vox_en_us_salli22i", 130, "english.lex", "", "Female", "en-US" }, /* 32 */
    { "eng", "USA", "Kimberly", "IVONA Kimberly", "com.ivona.tts.oem.voicebeta.eng.usa.kimberly", "libvoice_en_us_kimberly.so", -1, -1, "vox_en_us_kimberly22i", 136, "english.lex", "", "Female", "en-US" }, /* 33 */
    { "eng", "USA", "Kendra", "IVONA Kendra", "com.ivona.tts.oem.voicebeta.eng.usa.kendra", "libvoice_en_us_kendra.so", -1, -1, "vox_en_us_kendra22i", 132, "english.lex", "", "Female", "en-US" }, /* 34 */
    { "eng", "USA", "Justin", "IVONA Justin", "com.ivona.tts.oem.voicebeta.eng.usa.justin", "libvoice_en_us_justin.so", -1, -1, "vox_en_us_justin22i", 132, "english.lex", "", "Male", "en-US" }, /* 35 */
    { "eng", "USA", "Joey", "IVONA Joey", "com.ivona.tts.oem.voicebeta.eng.usa.joey", "libvoice_en_us_joey.so", -1, -1, "vox_en_us_joey22i", 128, "english.lex", "", "Male", "en-US" }, /* 36 */
    { "eng", "USA", "Jennifer", "IVONA Jennifer", "com.ivona.tts.oem.voicebeta.eng.usa.jennifer", "libvoice_en_us_jennifer.so", -1, -1, "vox_en_us_jennifer22i", 136, "english.lex", "", "Female", "en-US" }, /* 37 */
    { "eng", "USA", "Ivy", "IVONA Ivy", "com.ivona.tts.oem.voicebeta.eng.usa.ivy", "libvoice_en_us_ivy.so", -1, -1, "vox_en_us_ivy22i", 126, "english.lex", "", "Female", "en-US" }, /* 38 */
    { "eng", "USA", "Eric", "IVONA Eric", "com.ivona.tts.oem.voicebeta.eng.usa.eric", "libvoice_en_us_eric.so", -1, -1, "vox@2_en_us_eric22i", 132, "english.lex", "", "Male", "en-US" }, /* 39 */
    { "eng", "USA", "Chipmunk", "IVONA Chipmunk", "com.ivona.tts.oem.voicebeta.eng.usa.chipmunk", "libvoice_en_us_chipmunk.so", -1, -1, "vox_en_us_chipmunk22i", 136, "english.lex", "", "Male", "en-US" }, /* 40 */
    { "eng", "IND", "Raveena", "IVONA Raveena", "com.ivona.tts.oem.voicebeta.eng.ind.raveena", "libvoice_en_in_raveena.so", -1, -1, "vox_en_in_raveena22i", 134, "english.lex", "", "Female", "en-IN" }, /* 41 */
    { "eng", "GBR", "Emma", "IVONA Emma", "com.ivona.tts.oem.voicebeta.eng.gbr.emma", "libvoice_en_gb_emma.so", -1, -1, "vox_en_gb_emma22i", 128, "english.lex", "", "Female", "en-GB" }, /* 42 */
    { "eng", "GBR", "Brian", "IVONA Brian", "com.ivona.tts.oem.voicebeta.eng.gbr.brian", "libvoice_en_gb_brian.so", -1, -1, "vox_en_gb_brian22i", 130, "english.lex", "", "Male", "en-GB" }, /* 43 */
    { "eng", "GBR", "Amy", "IVONA Amy", "com.ivona.tts.oem.voicebeta.eng.gbr.amy", "libvoice_en_gb_amy.so", -1, -1, "vox_en_gb_amy22i", 126, "english.lex", "", "Female", "en-GB" }, /* 44 */
    { "eng", "AUS", "Russell", "IVONA Russell", "com.ivona.tts.oem.voicebeta.eng.aus.russell", "libvoice_en_au_russell.so", -1, -1, "vox_en_au_russell22i", 134, "english.lex", "", "Male", "en-AU" }, /* 45 */
    { "eng", "AUS", "Nicole", "IVONA Nicole", "com.ivona.tts.oem.voicebeta.eng.aus.nicole", "libvoice_en_au_nicole.so", -1, -1, "vox_en_au_nicole22i", 132, "english.lex", "", "Female", "en-AU" }, /* 46 */
    { "deu", "DEU", "Vicki", "IVONA Vicki", "com.ivona.tts.oem.voicebeta.deu.deu.vicki", "libvoice_de_vicki.so", -1, -1, "vox_de_vicki22i", 124, "german.lex", "", "Female", "de-DE" }, /* 47 */
    { "deu", "DEU", "Marlene", "IVONA Marlene", "com.ivona.tts.oem.voicebeta.deu.deu.marlene", "libvoice_de_marlene.so", -1, -1, "vox_de_marlene22i", 128, "german.lex", "", "Female", "de-DE" }, /* 48 */
    { "deu", "DEU", "Hans", "IVONA Hans", "com.ivona.tts.oem.voicebeta.deu.deu.hans", "libvoice_de_hans.so", -1, -1, "vox_de_hans22i", 122, "german.lex", "", "Male", "de-DE" }, /* 49 */
    { "dan", "DNK", "Naja", "IVONA Naja", "com.ivona.tts.oem.voicebeta.dan.dnk.naja", "libvoice_da_naja.so", -1, -1, "vox_da_naja22i", 122, "danish.lex", "", "Female", "da-DK" }, /* 50 */
    { "dan", "DNK", "Mads", "IVONA Mads", "com.ivona.tts.oem.voicebeta.dan.dnk.mads", "libvoice_da_mads.so", -1, -1, "vox_da_mads22i", 122, "danish.lex", "", "Male", "da-DK" }, /* 51 */
    { "cym", "GBR", "Gwyneth", "IVONA Gwyneth", "com.ivona.tts.oem.voicebeta.cym.gbr.gwyneth", "libvoice_cy_gwyneth.so", -1, -1, "vox_cy_gwyneth22i", 128, "welsh.lex", "", "Female", "cy-GB" }, /* 52 */
    { "cym", "GBR", "Geraint", "IVONA Geraint", "com.ivona.tts.oem.voicebeta.cym.gbr.geraint", "libvoice_cy_geraint.so", -1, -1, "vox_cy_geraint22i", 128, "welsh.lex", "", "Male", "cy-GB" }, /* 53 */
    { "pol", "POL", "Jacek", "IVONA Jacek", "com.ivona.tts.oem.voicebeta.pol.pol.jacek", "libvoice_pl_jacek.so", -1, -1, "vox_pl_jacek22i", 124, "polish.lex", "", "Male", "pl-PL" }, /* 54 */
};

/* 0x4ea0: returns the index-th registered voice, or NULL when index is out of range. */
const VoiceData *GetVoiceDataFunc(int index)
{
    if (index < 0)
        return NULL;
    size_t i = index;
    if (i >= sizeof(g_voice_table) / sizeof(g_voice_table[0]))
        return NULL;
    return &g_voice_table[i];
}

static VoiceRegistry g_registry = {
    sizeof(VoiceRegistry), 2, "sdk", sizeof(VoiceData), GetVoiceDataFunc,
};

/* 0x4ec0: entry point looked up by the engine with dlsym("v"). */
extern "C" VoiceRegistry *v(void)
{
    return &g_registry;
}

/* 0x4ecc: checks an installed voice: 0 when <dir><vox_file> of the voice with this package has
 * the size recorded in the table, 1 otherwise, -1 for bad arguments. An unknown package falls
 * through to the terminating entry (sic). */
extern "C" int h(const char *package, const char *dir)
{
    const VoiceData *d;
    struct stat st;
    int ret = -1;

    if (package != NULL && dir != NULL) {
        for (d = g_voice_table; d->language; d++)
            if (strcmp(package, d->package) == 0)
                break;
        std::string path;
        path = dir;
        path += d->vox_file;
        if (stat(path.c_str(), &st) == 0)
            ret = st.st_size != d->unk_24;
        else
            ret = 1;
    }
    return ret;
}
