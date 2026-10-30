/*
 * The homograph part-of-speech rules (0x3d908..0x482c8): en_word_pos picks the part of speech
 * of a word whose pronunciation depends on it (read/read, content, bass, lead, ...) from the
 * words around it, as the letter the lexicon keys entries with ("n", "v", "a", ... or "0" for
 * the default entry). About 780 neighbour-word tests over the word lists below.
 *
 * First version generated from the binary by tools/asm2c.py --readable (correct by
 * construction, still goto-style where the rules share code); checked against the lifted
 * original with the IVONA_CHECK_WORD_POS build (docs/notes/runnable-voice.md).
 */
#ifdef IVONA_CHECK_WORD_POS
#define en_word_pos en_word_pos_src   /* wrapped by tests/run/check_word_pos.c */
#endif
#include <stdlib.h>
#include <string.h>
#include "cst_item.h"
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_val.h"

cst_item *item_traverse(const cst_item *item, ...);
int phrase_contains_word(const cst_item *item, const char *const *words);
cst_item *phrase_start_token(const cst_item *item, const char *punc_break);
cst_item *phrase_end_token(const cst_item *item, const char *punc_break);
int cst_startswith_any(const char *s, const char *const *slist);
int cst_endswith_any(const char *s, const char *const *slist);
int en_lex_select_pos(const cst_item *item, const char *word, const char *s);
extern const char *_ctype_;
extern const cst_regex *const en_rx_be;
extern const cst_regex *const en_rx_have;

static const char *const en_wl_accidentally[] = {  /* 0x3c0f6c */
    "accidentally", "actually", "allegedly", "already", "also", "always", "apparently",
    "asynchronously", "barely", "blakely", "carefully", "certainly", "clearly", "constantly",
    "continuously", "deeply", "diligently", "eagerly", "easily", "effectively", "efficiently",
    "eventually", "ever", "evidently", "extremely", "faithfully", "finally", "freely", "frequently",
    "fully", "generally", "gravely", "hardly", "immediately", "indeed", "instantly", "just", "less",
    "literally", "magically", "manually", "merely", "more", "naturally", "nearly", "never",
    "normally", "not", "obviously", "often", "once", "only", "patiently", "perfectly", "perhaps",
    "potentially", "precisely", "previously", "probably", "properly", "quickly", "quietly", "quite",
    "really", "recently", "reliably", "repeatedly", "repetitively", "safely", "seldom",
    "simultaneously", "sometimes", "soon", "successfully", "suddenly", "surely", "technically",
    "therefore", "typically", "usually", "very", "widely", "willingly", "well", "yet", NULL
};
static const char *const en_wl_me[] = {  /* 0x3c10c4 */
    "me", "myself", "mine", "you", "yourself", "yours", "him", "his", "himself", "her", "hers",
    "herself", "it", "its", "itself", "us", "ours", "ourselves", "yourselves", "them", "their",
    "theirs", "themselves", "oneself", "thine", "thou", "yerself", NULL
};
static const char *const en_wl_read[] = {  /* 0x3c1134 */
    "read", "lipread", "misread", "proofread", "reread", "re-read", NULL
};
static const char *const en_wl_location[] = {  /* 0x3c1150 */
    "location", NULL
};
static const char *const en_wl_article[] = {  /* 0x3c1158 */
    "article", "banner", "card", "cover", "disclaimer", "lettering", "marquee", "message", "note",
    "page", "paragraphscreen", "sentence", "sign", "statement", "text", "writing", NULL
};
static const char *const en_wl_can[] = {  /* 0x3c119c */
    "can", "can't", "could", "couldn't", "does", "doesn't", "gonna", "may", "might", "must",
    "mustn't", "should", "shouldn't", "wanna", "would", "wouldn't", "will", "won't", "do", "don't",
    NULL
};
static const char *const en_wl_do[] = {  /* 0x3c11f0 */
    "do", "don't", "does", "doesn't", "did", "didn't", NULL
};
static const char *const en_wl_everybody[] = {  /* 0x3c120c */
    "everybody", "everyone", "her", "him", "me", "somebody", "someone", NULL
};
static const char *const en_wl_she[] = {  /* 0x3c122c */
    "she", "he", "it", NULL
};
static const char *const en_wl_can_2[] = {  /* 0x3c123c */
    "can", "could", "had", "have", "will", "would", NULL
};
static const char *const en_wl_anybody[] = {  /* 0x3c1258 */
    "anybody", "somebody", "nobody", "everybody", "anyone", "someone", "noone", "everyone", NULL
};
static const char *const en_wl_i[] = {  /* 0x3c127c */
    "i", "thou", "he", "she", "it", "we", "you", "ye", "y'all", "they", "who", NULL
};
static const char *const en_wl_and[] = {  /* 0x3c12ac */
    "and", "or", "than", NULL
};
static const char *const en_wl_do_2[] = {  /* 0x3c12bc */
    "do", "listen", "react", "write", NULL
};
static const char *const en_wl_did[] = {  /* 0x3c12d0 */
    "did", "done", "listened", "written", "wrote", NULL
};
static const char *const en_wl_blog[] = {  /* 0x3c12e8 */
    "blog", "journal", "newspaper", NULL
};
static const char *const en_wl_yesterday[] = {  /* 0x3c12f8 */
    "yesterday", NULL
};
static const char *const en_wl_capella[] = {  /* 0x3c1300 */
    "capella", "cappela", "cappella", "la", NULL
};
static const char *const en_wl_an[] = {  /* 0x3c1314 */
    "an", "class", "double", "grade", "group", "letter", "point", "straight", "team", "the", "type",
    NULL
};
static const char *const en_wl_grades[] = {  /* 0x3c1344 */
    "grades", "groups", "levels", NULL
};
static const char *const en_wl_content[] = {  /* 0x3c1354 */
    "content", "md", NULL
};
static const char *const en_wl_brooke[] = {  /* 0x3c1360 */
    "brooke", "carter", "fowler", "gallacher", "macleod", "mcbeal", "mccoist", "maxwell", "sheedy",
    "shewan", "walker", NULL
};
static const char *const en_wl_square[] = {  /* 0x3c1390 */
    "square", "squared", NULL
};
static const char *const en_wl_amp[] = {  /* 0x3c139c */
    "amp", "clarinet", "concerto", "drum", "effect", "effects", "fiddle", "guitar", "guitarists",
    "flute", "instrument", "pedal", "pedals", "piano", "player", "saxophone", "singer", "solo",
    "sound", "speaker", "synthesizer", "tone", "trumpet", "violin", "vocals", "voice", NULL
};
static const char *const en_wl_3c13fc[] = {  /* 0x3c13fc */
    "vocals", "voice", NULL
};
static const char *const en_wl_acoustic[] = {  /* 0x3c1408 */
    "acoustic", "double", "fender", "jazz", "keyboard", "techno", NULL
};
static const char *const en_wl_3c141c[] = {  /* 0x3c141c */
    "techno", NULL
};
static const char *const en_wl_bomb[] = {  /* 0x3c1424 */
    "bomb", "drum", NULL
};
static const char *const en_wl_fish[] = {  /* 0x3c1430 */
    "fish", "fishing", "fished", "dish", "fillet", NULL
};
static const char *const en_wl_3c143c[] = {  /* 0x3c143c */
    "dish", "fillet", NULL
};
static const char *const en_wl_edie[] = {  /* 0x3c1448 */
    "edie", "eddie", "arlisa", NULL
};
static const char *const en_wl_3c144c[] = {  /* 0x3c144c */
    "eddie", "arlisa", NULL
};
static const char *const en_wl_belfast[] = {  /* 0x3c1458 */
    "belfast", "bloemfontein", "boston", "cup", "dewsbury", "donegal", "farsley", "football",
    "glasgow", "oban", "park", "player", "players", "stalybridge", NULL
};
static const char *const en_wl_3c148c[] = {  /* 0x3c148c */
    "stalybridge", NULL
};
static const char *const en_wl_banc[] = {  /* 0x3c1494 */
    "banc", "bloc", "croute", "face", "garde", "masse", "passant", "plein", "pointe", "retard",
    "rose", "route", "scene", "suite", NULL
};
static const char *const en_wl_adderley[] = {  /* 0x3c14d0 */
    "adderley", "alpert", "andress", "brooks", "caen", "capozzi", "coleman", "edelman", "ellis",
    "gilbert", "gray", "grosch", "kane", "kaiwainui", "kelleher", "klein", "kohl", "magidson",
    "matthews", "orvis", "pederson", "peterson", "ritts", "robertson", "scharfman", "shriner",
    "sutter", "thomas", "trimpe", "voland", "wesson", NULL
};
static const char *const en_wl_buijs[] = {  /* 0x3c1550 */
    "buijs", "mayen", "neruda", "ullrich", NULL
};
static const char *const en_wl_3c155c[] = {  /* 0x3c155c */
    "ullrich", NULL
};
static const char *const en_wl_nisar[] = {  /* 0x3c1564 */
    "nisar", "mohammad", "muhammad", NULL
};
static const char *const en_wl_3c156c[] = {  /* 0x3c156c */
    "muhammad", NULL
};
static const char *const en_wl_ahmad[] = {  /* 0x3c1574 */
    "ahmad", "lalak", "hassan", "shakirullah", NULL
};
static const char *const en_wl_3c157c[] = {  /* 0x3c157c */
    "hassan", "shakirullah", NULL
};
static const char *const en_wl_la[] = {  /* 0x3c1588 */
    "la", "une", "de", "ma", "belle", NULL
};
static const char *const en_wl_de[] = {  /* 0x3c15a0 */
    "de", NULL
};
static const char *const en_wl_des[] = {  /* 0x3c15a8 */
    "des", "les", NULL
};
static const char *const en_wl_3c15ac[] = {  /* 0x3c15ac */
    "les", NULL
};
static const char *const en_wl_acetate[] = {  /* 0x3c15b4 */
    "acetate", "azide", "belt", "bromide", "chamber", "chloride", "chromate", "contamination",
    "crystal", "dioxide", "fluoride", "hydroxide", "iodide", "mine", "nitrate", "oxide", "paint",
    "poisoning", "room", "selenide", "sulfate", "sulfide", "telluride", "zirconate", "alloy",
    "bullet", "weights", "weight", "batteries", "battery", "atom", "isotope", "pipes", "pipe",
    "shot", NULL
};
static const char *const en_wl_3c163c[] = {  /* 0x3c163c */
    "shot", NULL
};
static const char *const en_wl_actor[] = {  /* 0x3c1644 */
    "actor", "actors", "actress", "actresses", "bass", "bassist", "bassists", "drummer", "drummers",
    "guitar", "guitars", "guitarist", "guitarists", "trumpet", "trumpets", "singer", "singers",
    "vocalist", "vocalists", NULL
};
static const char *const en_wl_3c168c[] = {  /* 0x3c168c */
    "vocalists", NULL
};
static const char *const en_wl_level[] = {  /* 0x3c1694 */
    "level", "levels", "concentration", "concentrations", NULL
};
static const char *const en_wl_3c169c[] = {  /* 0x3c169c */
    "concentration", "concentrations", NULL
};
static const char *const en_wl_get[] = {  /* 0x3c16a8 */
    "get", "gets", "gotten", "got", "getting", NULL
};
static const char *const en_wl_chemical[] = {  /* 0x3c16c0 */
    "chemical", "damaging", "effects", "harmful", "toxicity", NULL
};
static const char *const en_wl_3c16cc[] = {  /* 0x3c16cc */
    "harmful", "toxicity", NULL
};
static const char *const en_wl_john[] = {  /* 0x3c16d8 */
    "john", NULL
};
static const char *const en_wl_marissa[] = {  /* 0x3c16e0 */
    "marissa", NULL
};
static const char *const en_wl_canada[] = {  /* 0x3c16e8 */
    "canada", "saskatchewan", "city", "capital", NULL
};
static const char *const en_wl_in[] = {  /* 0x3c16fc */
    "in", "the", "a", NULL
};
static const char *const en_wl_bitter[] = {  /* 0x3c170c */
    "bitter", "blazing", "blistering", "damaging", "diplomatic", "fearsome", "fierce", "furious",
    "heated", "huge", "storming", "terrible", "violent", NULL
};
static const char *const en_wl_3c173c[] = {  /* 0x3c173c */
    "violent", NULL
};
static const char *const en_wl_alef[] = {  /* 0x3c1744 */
    "alef", "aleph", "ayin", "bet", "beth", "chaf", "dalet", "daleth", "fe", "gimel", "het", "heth",
    "kaf", "kaph", "lamed", "mem", "nun", "pe", "qof", "qoph", "resh", "sadhe", "samekh", "shin",
    "tav", "taw", "tet", "teth", "tsadi", "vav", "vet", "waw", "yod", "yodh", "zayin", "arabic",
    "aramaic", "hebrew", "phoenician", "semitic", "alphabet", "alphabets", NULL
};
static const char *const en_wl_bomb_2[] = {  /* 0x3c17f0 */
    "bomb", "brewing", "brewery", "cocktail", "drinks", "rice", "sakura", "set", "bottle", "cup",
    "kettle", "production", NULL
};
static const char *const en_wl_3c181c[] = {  /* 0x3c181c */
    "production", NULL
};
static const char *const en_wl_japanese[] = {  /* 0x3c1824 */
    "japanese", "nigori", "sakura", "red", "warm", "heated", "fine", NULL
};
static const char *const en_wl_3c183c[] = {  /* 0x3c183c */
    "fine", NULL
};
static const char *const en_wl_brewing[] = {  /* 0x3c1844 */
    "brewing", "drank", "drink", "drinks", "drinking", "drunk", NULL
};
static const char *const en_wl_the[] = {  /* 0x3c1860 */
    "the", "some", "more", "earth", NULL
};
static const char *const en_wl_3c186c[] = {  /* 0x3c186c */
    "earth", NULL
};
static const char *const en_wl_cup[] = {  /* 0x3c1874 */
    "cup", "bottle", "kettle", "quality", NULL
};
static const char *const en_wl_3c187c[] = {  /* 0x3c187c */
    "kettle", "quality", NULL
};
static const char *const en_wl_automobile[] = {  /* 0x3c1888 */
    "automobile", "car", "drive", NULL
};
static const char *const en_wl_3c188c[] = {  /* 0x3c188c */
    "car", "drive", NULL
};
static const char *const en_wl_alamaze[] = {  /* 0x3c1898 */
    "alamaze", "amos", "anthony", "black", "busshi", "carrington", "fixx", "hall", "hanson",
    "haring-smith", "kudo", "lacey", "murden", "praver", "scott", "sparks", "spelling", "thompson",
    "vega", "welles", NULL
};
static const char *const en_wl_armin[] = {  /* 0x3c18ec */
    "armin", "bernhard", "birte", "clemens", "heinz", "hermann", "johannes", "leopold", "shevah",
    "vladimir", "wilhelm", "wojciech", NULL
};
static const char *const en_wl_resign[] = {  /* 0x3c1920 */
    "resign", "resigns", "resigned", "resigning", NULL
};
static const char *const en_wl_3c192c[] = {  /* 0x3c192c */
    "resigning", NULL
};
static const char *const en_wl_the_2[] = {  /* 0x3c1934 */
    "the", "a", "an", NULL
};
static const char *const en_wl_3c193c[] = {  /* 0x3c193c */
    "an", NULL
};
static const char *const en_wl_me_2[] = {  /* 0x3c1944 */
    "me", "you", "her", "him", "us", "them", NULL
};
static const char *const en_wl_is[] = {  /* 0x3c1960 */
    "is", "has", "does", NULL
};
static const char *const en_wl_france[] = {  /* 0x3c1970 */
    "france", "summit", "treaty", NULL
};
static const char *const en_wl_in_2[] = {  /* 0x3c1980 */
    "in", "from", "to", NULL
};
static const char *const en_wl_king[] = {  /* 0x3c1990 */
    "king", "language", "state", "tribe", "women", NULL
};
static const char *const en_wl_3c199c[] = {  /* 0x3c199c */
    "tribe", "women", NULL
};
static const char *const en_wl_abbey[] = {  /* 0x3c19a8 */
    "abbey", "palace", "pinetum", NULL
};
static const char *const en_wl_3c19ac[] = {  /* 0x3c19ac */
    "palace", "pinetum", NULL
};
static const char *const en_wl_horse[] = {  /* 0x3c19b8 */
    "horse", "thoroughbreds", NULL
};
static const char *const en_wl_3c19bc[] = {  /* 0x3c19bc */
    "thoroughbreds", NULL
};
static const char *const en_wl_occidental[] = {  /* 0x3c19c4 */
    "occidental", "oriental", "island", NULL
};
static const char *const en_wl_3c19cc[] = {  /* 0x3c19cc */
    "island", NULL
};
static const char *const en_wl_in_3[] = {  /* 0x3c19d4 */
    "in", "of", "los", "from", "to", NULL
};
static const char *const en_wl_colorado[] = {  /* 0x3c19ec */
    "colorado", "georgia", "mississippi", "ohio", "york", NULL
};
static const char *const en_wl_3c19fc[] = {  /* 0x3c19fc */
    "york", NULL
};
static const char *const en_wl_co[] = {  /* 0x3c1a04 */
    "CO", "GA", "MS", "NY", "OH", NULL
};
static const char *const en_wl_kentucky[] = {  /* 0x3c1a1c */
    "kentucky", NULL
};
static const char *const en_wl_boulder[] = {  /* 0x3c1a24 */
    "boulder", "jefferson", "winston", "lawrence", "stark", NULL
};
static const char *const en_wl_tesfaye[] = {  /* 0x3c1a3c */
    "tesfaye", NULL
};
static const char *const en_wl_england[] = {  /* 0x3c1a44 */
    "england", NULL
};
static const char *const en_wl_alabama[] = {  /* 0x3c1a4c */
    "alabama", NULL
};
static const char *const en_wl_moray[] = {  /* 0x3c1a54 */
    "moray", "scotland", "texas", "tx", "uk", NULL
};
static const char *const en_wl_il[] = {  /* 0x3c1a6c */
    "il", "illinois", NULL
};
static const char *const en_wl_bastrop[] = {  /* 0x3c1a78 */
    "bastrop", "cathedral", "kingdom", "moray", "scotland", "texas", "travis", "tx", "uk", NULL
};
static const char *const en_wl_cook[] = {  /* 0x3c1aa0 */
    "cook", "kane", "il", "illinois", NULL
};
static const char *const en_wl_3c1aac[] = {  /* 0x3c1aac */
    "illinois", NULL
};
static const char *const en_wl_england_2[] = {  /* 0x3c1ab4 */
    "england", NULL
};
static const char *const en_wl_penn[] = {  /* 0x3c1abc */
    "penn", "pennsylvania", NULL
};
static const char *const en_wl_berkshire[] = {  /* 0x3c1ac8 */
    "berkshire", "england", "kansas", "massachusetts", "michigan", "minnesota", "ohio",
    "pennsylvania", "vermont", "township", "uk", "united", NULL
};
static const char *const en_wl_borough[] = {  /* 0x3c1afc */
    "borough", "city", "community", "county", "town", NULL
};
static const char *const en_wl_3c1b0c[] = {  /* 0x3c1b0c */
    "town", NULL
};
static const char *const en_wl_florida[] = {  /* 0x3c1b14 */
    "florida", "everybody's", "children's", NULL
};
static const char *const en_wl_3c1b1c[] = {  /* 0x3c1b1c */
    "children's", NULL
};
static const char *const en_wl_adam[] = {  /* 0x3c1b24 */
    "adam", "alain", "cousin", "de", "fresneau", "georges", "rictus", "tabourot", NULL
};
static const char *const en_wl_3c1b3c[] = {  /* 0x3c1b3c */
    "rictus", "tabourot", NULL
};
static const char *const en_wl_uh[] = {  /* 0x3c1b48 */
    "uh", NULL
};
static const char *const en_wl_roh[] = {  /* 0x3c1b50 */
    "roh", NULL
};
static const char *const en_wl_ehud[] = {  /* 0x3c1b58 */
    "ehud", "hans", "laszlo", NULL
};
static const char *const en_wl_3c1b5c[] = {  /* 0x3c1b5c */
    "hans", "laszlo", NULL
};
static const char *const en_wl_andrzej[] = {  /* 0x3c1b68 */
    "andrzej", "grzegorz", "jacek", "jerzy", "joanna", "piotr", NULL
};
static const char *const en_wl_3c1b7c[] = {  /* 0x3c1b7c */
    "piotr", NULL
};
static const char *const en_wl_aleksander[] = {  /* 0x3c1b84 */
    "aleksander", "andrzej", "badkowski", "gardocki", "garlicki", "janerka", "jeczmyk", "kaczynski",
    "kolakowski", "kowalski", "koziejowski", "kuropatwinski", "lasko", "lewandowski", "mackiewicz",
    "majewski", "ordon", "owron", "piasecki", "pijanowski", "poznan", "rzewuski", "rypin",
    "stanislaw", "szczucki", "szymanczyk", "trzeciakowski", "walesa", "woszczerowicz", NULL
};
static const char *const en_wl_baxter[] = {  /* 0x3c1bfc */
    "baxter", "blank", "brown", "claypool", "costello", "dawson", "eaves", "edwards", "ferdinand",
    "goble", "kaufman", "mccann", "mckeown", "miles", "mills", "palmer", "paul", "reed", "shannon",
    "stroud", "tanyuk", "tremayne", NULL
};
static const char *const en_wl_3c1c4c[] = {  /* 0x3c1c4c */
    "tanyuk", "tremayne", NULL
};
static const char *const en_wl_vogels[] = {  /* 0x3c1c58 */
    "vogels", "herzog", NULL
};
static const char *const en_wl_3c1c5c[] = {  /* 0x3c1c5c */
    "herzog", NULL
};
static const char *const en_wl_zero[] = {  /* 0x3c1c64 */
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven",
    "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
    "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety", "hundred",
    "thousand", "million", "billion", "trillion", NULL
};
static const char *const en_wl_and_2[] = {  /* 0x3c1cec */
    "and", "or", "if", "than", "by", NULL
};
static const char *const en_wl_3c1cfc[] = {  /* 0x3c1cfc */
    "by", NULL
};
static const char *const en_wl_get_2[] = {  /* 0x3c1d04 */
    "get", "gets", "gotten", "got", "getting", "become", "becomes", "became", "becoming", "feel",
    "feels", "felt", "feeling", "grow", "grows", "grew", "grown", "growing", "and", "or", NULL
};
static const char *const en_wl_3c1d4c[] = {  /* 0x3c1d4c */
    "and", "or", NULL
};
static const char *const en_wl_help[] = {  /* 0x3c1d58 */
    "help", "helps", "helped", "helping", NULL
};
static const char *const en_wl_cable[] = {  /* 0x3c1d6c */
    "cable", "cables", "call", "calls", "computer", "computers", "device", "devices", "error",
    "errors", "item", "items", "letter", "letters", "message", "messages", "option", "options",
    "package", "packages", "parcel", "parcels", "passenger", "passengers", "phone", "phones",
    "query", "queries", "request", "requests", "send", "sends", "signal", "signals", "task",
    "tasks", "tube", "tubes", NULL
};
static const char *const en_wl_3c1dfc[] = {  /* 0x3c1dfc */
    "tube", "tubes", NULL
};
static const char *const en_wl_ship[] = {  /* 0x3c1e08 */
    "ship", "ships", "ship's", "boat", "boats", "boat's", "canoe", "canoes", "canoe's", NULL
};
static const char *const en_wl_orientation[] = {  /* 0x3c1e30 */
    "orientation", "x", "y", "z", "axis", "roll", "yaw", "pitch", "parallel", "geometric",
    "vertical", "angle", "graph", "graphs", "constant", NULL
};
static const char *const en_wl_ordinate[] = {  /* 0x3c1e70 */
    "ordinate", "ordinates", "rotat", "vector", "measurement", "dimensional", "center", "centr",
    "symmetr", "asymmetr", "value", "system", "fold", NULL
};
static const char *const en_wl_3c1e9c[] = {  /* 0x3c1e9c */
    "system", "fold", NULL
};
static const char *const en_wl_listen[] = {  /* 0x3c1ea8 */
    "listen", "listened", "listening", "bind", "binded", "binding", "travel", "travelling",
    "travelled", "welcome", "welcoming", "access", "approach", "talk", " talking", "talked",
    "introduction", "reply", "replied", "back", "access", "guide", "goodbye", "connect",
    "connected", "connecting", NULL
};
static const char *const en_wl_3c1f0c[] = {  /* 0x3c1f0c */
    "connecting", NULL
};
static const char *const en_wl_road[] = {  /* 0x3c1f14 */
    "road", "roads", "path", "paths", NULL
};
static const char *const en_wl_3c1f1c[] = {  /* 0x3c1f1c */
    "path", "paths", NULL
};
static const char *const en_wl_do_3[] = {  /* 0x3c1f28 */
    "do", "re", "mi", "fa", "so", "la", "ti", "sol", "si", NULL
};
static const char *const en_wl_mi[] = {  /* 0x3c1f50 */
    "mi", "fa", "la", "ti", "sol", "si", NULL
};
static const char *const en_wl_do_4[] = {  /* 0x3c1f6c */
    "Do", "Re", "Mi", "Fa", "So", "La", "Ti", "Sol", "Si", NULL
};
static const char *const en_wl_3c1f74[] = {  /* 0x3c1f74 */
    "Mi", "Fa", "So", "La", "Ti", "Sol", "Si", NULL
};
static const char *const en_wl_amoris[] = {  /* 0x3c1f94 */
    "amoris", "cedit", "communes", "datus", "dominicus", "excrescens", "fasti", "feriati",
    "gratias", "intercisi", "irae", "juridicus", "legitmus", "nefasti", "non", "pacis", "solis",
    "solaris", "utiles", NULL
};
static const char *const en_wl_stay[] = {  /* 0x3c1fe4 */
    "stay", "stays", "stayed", "staying", NULL
};
static const char *const en_wl_good[] = {  /* 0x3c1ff8 */
    "good", "great", "excellent", NULL
};
static const char *const en_wl_ahi[] = {  /* 0x3c2008 */
    "ahi", "tuna", "octopus", "tako", "salmon", "tofu", "avocado", "hawaiian", NULL
};
static const char *const en_wl_3c2014[] = {  /* 0x3c2014 */
    "tako", "salmon", "tofu", "avocado", "hawaiian", NULL
};
static const char *const en_wl_have[] = {  /* 0x3c202c */
    "have", "has", "had", NULL
};
static const char *const en_wl_3c2034[] = {  /* 0x3c2034 */
    "had", NULL
};
static const char *const en_wl_music[] = {  /* 0x3c203c */
    "music", "production", "productions", "entertainment", "show", "shows", "tv", "television",
    "televisions", "concert", "concerts", "score", "scores", "match", "matches", "chat", "chats",
    "cam", "cams", "camera", "cameras", "search", "searches", "racing", "studio", "studios",
    "broadcast", "broadcasts", "broadcasting", "webcast", "webcasts", "webcasting", "rock",
    "expectancy", "ammunition", "arcade", "performance", "performances", "session", "sessions",
    "action", "version", "versions", "video", "radio", "service", "services", "update", "updates",
    NULL
};
static const char *const en_wl_3c2044[] = {  /* 0x3c2044 */
    "productions", "entertainment", "show", "shows", "tv", "television", "televisions", "concert",
    "concerts", "score", "scores", "match", "matches", "chat", "chats", "cam", "cams", "camera",
    "cameras", "search", "searches", "racing", "studio", "studios", "broadcast", "broadcasts",
    "broadcasting", "webcast", "webcasts", "webcasting", "rock", "expectancy", "ammunition",
    "arcade", "performance", "performances", "session", "sessions", "action", "version", "versions",
    "video", "radio", "service", "services", "update", "updates", NULL
};
static const char *const en_wl_sold[] = {  /* 0x3c2104 */
    "sold", "keep", "keeping", NULL
};
static const char *const en_wl_announced[] = {  /* 0x3c2114 */
    "announced", "broadcast", "broadcasted", "performed", "shown", "televised", NULL
};
static const char *const en_wl_i_2[] = {  /* 0x3c2130 */
    "i", "you", "we", "they", NULL
};
static const char *const en_wl_3c2134[] = {  /* 0x3c2134 */
    "you", "we", "they", NULL
};
static const char *const en_wl_afore[] = {  /* 0x3c2144 */
    "afore", "after", "against", "among", "amongst", "at", "beside", "for", "from", "into", "of",
    "onto", "per", "unto", "via", "with", "without", NULL
};
static const char *const en_wl_free[] = {  /* 0x3c218c */
    "free", "good", "long", "online", "professional", "sample", "short", NULL
};
static const char *const en_wl_3c2194[] = {  /* 0x3c2194 */
    "long", "online", "professional", "sample", "short", NULL
};
static const char *const en_wl_beautiful[] = {  /* 0x3c21ac */
    "beautiful", "black", "collared", "golden", "little", "lonesome", "morning", "mourning",
    "peace", "peaceful", "prestigious", "river", "soiled", "turtle", "white", "white-winged",
    "wooden", "wounded", NULL
};
static const char *const en_wl_3c21b4[] = {  /* 0x3c21b4 */
    "collared", "golden", "little", "lonesome", "morning", "mourning", "peace", "peaceful",
    "prestigious", "river", "soiled", "turtle", "white", "white-winged", "wooden", "wounded", NULL
};
static const char *const en_wl_harvested[] = {  /* 0x3c21f8 */
    "harvested", "fresh", "dairy", "good", "summer", "local", "find", "offering", "canned", "green",
    "organic", "eat", "seafood", "produce", "quality", "high-quality", "buy", "buys", "bought",
    "buying", "sell", "sold", "sells", "selling", NULL
};
static const char *const en_wl_3c2204[] = {  /* 0x3c2204 */
    "good", "summer", "local", "find", "offering", "canned", "green", "organic", "eat", "seafood",
    "produce", "quality", "high-quality", "buy", "buys", "bought", "buying", "sell", "sold",
    "sells", "selling", NULL
};
static const char *const en_wl_fruit[] = {  /* 0x3c225c */
    "fruit", "livestock", "goods", "food", "fish", "seafood", "eggs", "meat", "meats", NULL
};
static const char *const en_wl_3c2264[] = {  /* 0x3c2264 */
    "goods", "food", "fish", "seafood", "eggs", "meat", "meats", NULL
};
static const char *const en_wl_overall[] = {  /* 0x3c2284 */
    "overall", "towering", "similar", "into", "any", "take", "takes", "took", "taken", "taking",
    NULL
};
static const char *const en_wl_body[] = {  /* 0x3c22b0 */
    "body", "boot", "butcher", "car", "french", "kiwi", "shoe", "nail", "art", NULL
};
static const char *const en_wl_3c22b4[] = {  /* 0x3c22b4 */
    "boot", "butcher", "car", "french", "kiwi", "shoe", "nail", "art", NULL
};
static const char *const en_wl_window[] = {  /* 0x3c22d8 */
    "window", "panel", "tab", NULL
};
static const char *const en_wl_advocate[] = {  /* 0x3c22e8 */
    "advocate", "advocates", "articulate", "articulates", "associate", "associates", "attribute",
    "attributes", "compact", "compacts", "conduct", "conducts", "conscripts", "console", "consoles",
    "consorts", "coordinate", "coordinates", "digest", "discharge", "download", "downloads",
    "duplicate", "duplicates", "estimate", "estimates", "export", "exports", "graduate",
    "graduates", "implant", "implants", "import", "imports", "intrigue", "intrigues", "offset",
    "overhang", "override", "overrides", "permit", "permits", "postulate", "postulates", "protest",
    "protests", "refuse", "subordinate", "subordinates", "torment", "torments", NULL
};
static const char *const en_wl_3c22f4[] = {  /* 0x3c22f4 */
    "articulates", "associate", "associates", "attribute", "attributes", "compact", "compacts",
    "conduct", "conducts", "conscripts", "console", "consoles", "consorts", "coordinate",
    "coordinates", "digest", "discharge", "download", "downloads", "duplicate", "duplicates",
    "estimate", "estimates", "export", "exports", "graduate", "graduates", "implant", "implants",
    "import", "imports", "intrigue", "intrigues", "offset", "overhang", "override", "overrides",
    "permit", "permits", "postulate", "postulates", "protest", "protests", "refuse", "subordinate",
    "subordinates", "torment", "torments", NULL
};
static const char *const en_wl_axes[] = {  /* 0x3c23b8 */
    "axes", "blanches", "implements", "perfume", "perfumes", "proceeds", "upset", NULL
};
static const char *const en_wl_3c23c4[] = {  /* 0x3c23c4 */
    "perfume", "perfumes", "proceeds", "upset", NULL
};
static const char *const en_wl_were[] = {  /* 0x3c23d8 */
    "were", NULL
};
static const char *const en_wl_major[] = {  /* 0x3c23e0 */
    "major", "main", "optical", "three", "principal", "spin", NULL
};
static const char *const en_wl_3c23e4[] = {  /* 0x3c23e4 */
    "main", "optical", "three", "principal", "spin", NULL
};
static const char *const en_wl_3d8060[] = {  /* 0x3d8060 */
    "brut", "pobol", "craig", "ortega", "gymraeg", "gramadeg", "castell", "pen", "englynion", NULL
};
static const char *const en_wl_3d8070[] = {  /* 0x3d8070 */
    "gymraeg", "gramadeg", "castell", "pen", "englynion", NULL
};
static const char *const en_wl_3d8080[] = {  /* 0x3d8080 */
    "englynion", NULL
};
static const char *const en_wl_3d8088[] = {  /* 0x3d8088 */
    "fenni", "brenhinedd", "gaer", "gododdin", NULL
};
static const char *const en_wl_3d809c[] = {  /* 0x3d809c */
    "suffolk", "super", "big", "street", "generation", "the", NULL
};
static const char *const en_wl_3d80ac[] = {  /* 0x3d80ac */
    "generation", "the", NULL
};
static const char *const en_wl_3d80b8[] = {  /* 0x3d80b8 */
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven",
    "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
    "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety", NULL
};
static const char *const en_wl_3d812c[] = {  /* 0x3d812c */
    "their", "the", "our", "own", NULL
};
static const char *const en_wl_3d8140[] = {  /* 0x3d8140 */
    "of", "are", "were", "have", "than", "would", "will", NULL
};
static const char *const en_wl_3d8150[] = {  /* 0x3d8150 */
    "than", "would", "will", NULL
};
static const char *const en_wl_3d8160[] = {  /* 0x3d8160 */
    "who", "he", "she", NULL
};
static const char *const en_wl_3d8170[] = {  /* 0x3d8170 */
    "in", "on", "with", "there", "here", "up", "near", "alone", "next", "within", "under", NULL
};
static const char *const en_wl_3d8180[] = {  /* 0x3d8180 */
    "here", "up", "near", "alone", "next", "within", "under", NULL
};
static const char *const en_wl_3d8190[] = {  /* 0x3d8190 */
    "next", "within", "under", NULL
};

/* 0x3d908 arm: generated by tools/asm2c.py --readable */
const char *en_word_pos(const cst_item *word)
{
    cst_item *it1 = NULL;
    cst_item *it14 = NULL;
    cst_item *it21 = NULL;
    cst_item *it22 = NULL;
    cst_item *it23 = NULL;
    cst_item *it3 = NULL;
    cst_item *it_r0 = NULL;
    cst_item *it_r4 = NULL;
    cst_item *it_r5 = NULL;
    cst_item *it_r6 = NULL;
    cst_item *it_r9 = NULL;
    int n13 = 0;
    int n18 = 0;
    int n19 = 0;
    int n2 = 0;
    int n6 = 0;
    int n9 = 0;
    int n_Sc = 0;
    int n_r2 = 0;
    int n_r3 = 0;
    void *p_r0 = NULL;
    void *p_r4 = NULL;
    const cst_regex *rx_r5 = NULL;
    const cst_regex *rx_r6 = NULL;
    const char *s10 = NULL;
    const char *s11 = NULL;
    const char *s15 = NULL;
    const char *s17 = NULL;
    const char *s20 = NULL;
    const char *s4 = NULL;
    const char *s5 = NULL;
    const char *s7 = NULL;
    const char *s8 = NULL;
    const char *s_Sc = NULL;
    const char *s_r0 = NULL;
    const char *s_r3 = NULL;
    const char *s_r4 = NULL;
    const char *s_r5 = NULL;
    const char *s_r6 = NULL;
    const char *s_r9 = NULL;
    const cst_val *v12 = NULL;
    const cst_val *v16 = NULL;
    const cst_val *v_r0 = NULL;

    it1 = item_as(word, 3);
    it_r4 = it1;
    if (strcmp(ffeature_string(item_traverse(it1, 0), "gpos"), "content") != 0)
        goto L_3da50;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "please") != 0)
        goto L_3d990;
    return "v";
L_3d990:
    n2 = cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally);
    it_r6 = 0;
    if (!n2)
        it_r6 = it_r4;
    if (n2)
        goto L_3dd74;
L_3d9c0:
    if (strcmp(ffeature_string(item_traverse(it_r6, 0x103, 0), "name"), "not") == 0)
        goto L_3dd64;
L_3d9f0:
    if (strcmp(ffeature_string(item_traverse(it_r6, 0x103, 0), "name"), "let's") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r6, 0x104, 0), "name"), "let") == 0)
        goto L_3ddf4;
L_3da50:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "read") != 0)
        goto L_3dbc0;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_location))
        return "t";
L_3db34:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "which") == 0)
        return "t";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3dbc0;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "which") == 0)
        return "t";
L_3dbc0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0), "name"), en_wl_read))
        goto L_3dc8c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "a") != 0)
        goto L_3dc48;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "good") == 0)
        return "0";
L_3dc48:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") != 0)
        goto L_3de24;
    return "0";
L_3dc8c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "a") != 0)
        goto L_3dce4;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_capella))
        goto L_3ddbc;
    return "p";
L_3dce4:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "abe") == 0)
        goto L_3dd84;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "aesop") != 0)
        goto L_3e080;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "rock") == 0)
        return "1";
    return "0";
L_3dd64:
    it_r6 = item_prev(it_r6);
    goto L_3d9f0;
L_3dd74:
    it_r6 = item_prev(it_r4);
    goto L_3d9c0;
L_3dd84:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "shinzo") != 0)
        goto L_3e160;
    return "1";
L_3ddbc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "while") != 0)
        goto L_3e290;
    return "r";
L_3ddf4:
    if (!cst_member_string(ffeature_string(item_traverse(it_r6, 0x103, 0), "name"), en_wl_me))
        goto L_3da50;
    return "v";
L_3de24:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "rather") != 0)
        goto L_3de7c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "or") != 0)
        goto L_3e4cc;
L_3de7c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_can))
        return "0";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_can))
        goto L_3e41c;
L_3dee0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "have") != 0)
        goto L_3df40;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_everybody))
        return "0";
L_3df40:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_do))
        goto L_3dfa0;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_she))
        return "0";
L_3dfa0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), en_wl_do))
        goto L_3e004;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "not") == 0)
        goto L_3e72c;
L_3e004:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "md") == 0)
        return "0";
    rx_r6 = en_rx_have;
    if (!cst_regex_match(en_rx_have, ffeature_string(item_traverse(it_r4, 0x103, 0), "name")))
        goto L_3ea70;
    return "t";
L_3e080:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "ally") != 0)
        goto L_3e0cc;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_brooke))
        return "1";
L_3e0cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "arie") != 0)
        goto L_3e124;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "india") == 0)
        return "1";
L_3e124:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "august") != 0)
        goto L_3e4fc;
    return "n";
L_3e160:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "minister") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "shinzo") == 0)
        return "1";
L_3e1b4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "kiro") != 0)
        goto L_3e210;
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "KIRO") == 0)
        return "1";
L_3e210:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "ya") != 0)
        goto L_3e24c;
    if (!item_prev(it_r4))
        goto L_3f13c;
L_3e24c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0), "name"), en_wl_advocate))
        goto L_3e6fc;
    return "n";
L_3e290:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "A") != 0)
        goto L_3e308;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_an))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_grades))
        return "n";
L_3e308:
    if (!item_next(it_r4))
        return "n";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), en_wl_content))
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "nsw"), "hyphenated-word") != 0)
        return "r";
    it3 = item_parent(item_as(it_r4, 1));
    it_r5 = it3;
    if (it_r4 == item_as(item_daughter(it3), 3))
        return "n";
    if (it_r4 == item_as(item_last_daughter(it_r5), 3))
        return "n";
    s4 = item_feat_string(item_prev(it_r4), "name");
    s5 = item_feat_string(item_next(it_r4), "name");
    s_r4 = s5;
    if (strlen(s4) <= 1)
        return "n";
    if (strlen(s_r4) > 1)
        return "r";
    return "n";
L_3e41c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "not") == 0)
        return "0";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "then") == 0)
        return "0";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "now") == 0)
        return "0";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3dee0;
    return "0";
L_3e4cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "but") == 0)
        goto L_3de7c;
    return "0";
L_3e4fc:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "chi") != 0)
        goto L_3e560;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_square))
        return "n";
    return "1";
L_3e560:
    if (cst_startswith(ffeature_string(item_traverse(it_r0, 0), "name"), "chi-square"))
        return "n";
    if (strstr(ffeature_string(item_traverse(it_r4, 0), "name"), "-chi-square"))
        return "n";
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "bass") != 0)
        goto L_3f158;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_amp))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_acoustic))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_bomb))
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "sea") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "swam") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "swims") == 0)
        return "1";
    if (!phrase_contains_word(item_parent(it_r4), en_wl_fish))
        goto L_3e1b4;
    return "1";
L_3e6fc:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0), "name"), en_wl_axes))
        return "v";
    return "0";
L_3e72c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_she))
        goto L_3e004;
    return "0";
L_3ea70:
    rx_r5 = en_rx_be;
    if (cst_regex_match(en_rx_be, ffeature_string(item_traverse(it_r4, 0x103, 0), "name")))
        return "t";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3eb24;
    if (cst_regex_match(rx_r6, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        return "t";
    if (cst_regex_match(rx_r5, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        return "t";
L_3eb24:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3eb84;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_accidentally))
        goto L_3f410;
L_3eb84:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "pps") != 0)
        goto L_3ec0c;
    if (cst_regex_match(rx_r6, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        return "t";
    if (cst_regex_match(rx_r5, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        return "t";
L_3ec0c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3ec70;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "pps") == 0)
        goto L_3f468;
L_3ec70:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "pps") != 0)
        goto L_3eccc;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "just") == 0)
        return "t";
L_3eccc:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_she))
        goto L_3ed2c;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_can_2))
        return "t";
L_3ed2c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3ed8c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_she))
        goto L_3f4bc;
L_3ed8c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_anybody))
        goto L_3edec;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_can_2))
        return "t";
L_3edec:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3ee4c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_anybody))
        goto L_3f4e8;
L_3ee4c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "being") == 0)
        return "t";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "by") == 0)
        goto L_3f514;
L_3eeac:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "page") == 0)
        return "t";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_and))
        goto L_3f544;
L_3ef10:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_do_2))
        goto L_3ef70;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_and))
        return "0";
L_3ef70:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_and))
        goto L_3efd0;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_did))
        return "t";
L_3efd0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_did))
        goto L_3f030;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_and))
        return "t";
L_3f030:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "(read") != 0)
        goto L_3f1a8;
    return "0";
L_3f080:
    p_r4 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "as") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "to") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "anything") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "what") != 0)
        goto L_47070;
    return "v";
L_3f13c:
    if (item_next(it_r4))
        goto L_3e24c;
    return "i";
L_3f158:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "baton") != 0)
        goto L_3f2c8;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "rouge") != 0)
        goto L_3e1b4;
    return "1";
L_3f1a8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "having") == 0)
        return "t";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "having") == 0)
        goto L_3f570;
L_3f200:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "a") != 0)
        goto L_3f3e0;
L_3f234:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_blog))
        return "t";
L_3f268:
    it_r5 = item_daughter(item_parent(it_r4));
L_3f28c:
    it_r0 = it_r5;
    if (!it_r5)
        goto L_3f320;
    it_r0 = it_r5;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0), "name"), en_wl_yesterday))
        return "t";
    it_r5 = item_next(it_r0);
    goto L_3f28c;
L_3f2c8:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "brickell") != 0)
        goto L_3f390;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), en_wl_edie))
        goto L_3e1b4;
    return "1";
L_3f320:
    if (cst_member_string(ffeature_string(item_traverse(item_daughter(phrase_start_token(item_parent(it_r4), 0)), 0), "name"), en_wl_yesterday))
        return "t";
    if (!phrase_contains_word(item_parent(it_r4), en_wl_were))
        goto L_3e1b4;
    return "t";
L_3f390:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "clara") != 0)
        goto L_3f59c;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "nunes") != 0)
        goto L_3e1b4;
    return "1";
L_3f3e0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "an") == 0)
        goto L_3f234;
    goto L_3f268;
L_3f410:
    if (cst_regex_match(rx_r6, ffeature_string(item_traverse(it_r4, 0x105, 0), "name")))
        return "t";
    if (!cst_regex_match(rx_r5, ffeature_string(item_traverse(it_r4, 0x105, 0), "name")))
        goto L_3eb84;
    return "t";
L_3f468:
    if (cst_regex_match(rx_r6, ffeature_string(item_traverse(it_r4, 0x105, 0), "name")))
        return "t";
    if (!cst_regex_match(rx_r5, ffeature_string(item_traverse(it_r4, 0x105, 0), "name")))
        goto L_3ec70;
    return "t";
L_3f4bc:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), en_wl_can_2))
        goto L_3ed8c;
    return "t";
L_3f4e8:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), en_wl_can_2))
        goto L_3ee4c;
    return "t";
L_3f514:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_i))
        goto L_3eeac;
    return "t";
L_3f544:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_do_2))
        goto L_3ef10;
    return "0";
L_3f570:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_3f200;
    return "t";
L_3f59c:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "celtic") != 0)
        goto L_3f62c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_belfast))
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_belfast))
        goto L_3e1b4;
    return "1";
L_3f62c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "cetera") != 0)
        goto L_3f6b8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "et") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "ec") != 0)
        goto L_3e1b4;
    return "1";
L_3f6b8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "content") != 0)
        goto L_3f728;
    rx_r5 = en_rx_be;
    if (!cst_regex_match(en_rx_be, ffeature_string(item_traverse(it_r4, 0x103, 0), "name")))
        goto L_3f7d8;
    return "v";
L_3f728:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "des") == 0)
        goto L_3f988;
L_3f750:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "does") != 0)
        goto L_3f924;
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "any") != 0)
        goto L_3f860;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x104, 0), "name"), "if") == 0)
        return "v";
    return "n";
L_3f7d8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "how") != 0)
        goto L_3e1b4;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_i))
        goto L_3e1b4;
    if (!cst_regex_match(rx_r5, ffeature_string(item_traverse(it_r4, 0x101, 0), "name")))
        goto L_3e1b4;
    return "v";
L_3f860:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "the") != 0)
        goto L_3faa8;
L_3f888:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "det") == 0)
        return "v";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_me))
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "not") == 0)
        return "v";
    return "n";
L_3f924:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "en") != 0)
        goto L_3f9bc;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_banc))
        goto L_3e1b4;
    return "p";
L_3f988:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "moines") != 0)
        goto L_3f750;
    return "2";
L_3f9bc:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "evening") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "fine") == 0)
        goto L_3fdec;
L_3fa08:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "grown-up") == 0)
        return "n";
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "herb") != 0)
        goto L_3fad8;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_adderley))
        return "n";
    return "1";
L_3faa8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "some") == 0)
        goto L_3f888;
    return "v";
L_3fad8:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "instinct") == 0)
        return "n";
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "irvine") != 0)
        goto L_3fd44;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "california") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "u.c.") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "university") == 0)
        goto L_3fcf4;
L_3fbac:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "temperature") != 0)
        goto L_3fc08;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "in") == 0)
        return "1";
L_3fc08:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "forecast") != 0)
        goto L_3fc64;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "for") == 0)
        return "1";
L_3fc64:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Irvine") != 0)
        goto L_3e1b4;
    n6 = ((const signed char *)(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name")))[0];
    n_r3 = n6;
    if (n6 == -1)
        goto L_3e1b4;
    if (!(((const unsigned char *)((_ctype_ + (n_r3 & 255))))[1] & 1))
        goto L_3e1b4;
    return "2";
L_3fcf4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "of") != 0)
        goto L_3fbac;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "california") != 0)
        goto L_3fbac;
    return "1";
L_3fd44:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "jan") != 0)
        goto L_3fe18;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_buijs))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_nisar))
        return "2";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_ahmad))
        goto L_3e1b4;
    return "2";
L_3fdec:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "gael") != 0)
        goto L_3fa08;
    return "1";
L_3fe18:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "jean") != 0)
        goto L_3fea0;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "wyclef") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "michel") == 0)
        return "1";
    return "n";
L_3fea0:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "vie") != 0)
        goto L_3ff34;
L_3fec4:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_la))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_de))
        return "n";
    return "v";
L_3ff34:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "vie's") == 0)
        goto L_3fec4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "viola") == 0)
        goto L_3ffe8;
L_3ff84:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "cent") != 0)
        goto L_40014;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_des))
        return "1";
    return "n";
L_3ffe8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "davis") != 0)
        goto L_3ff84;
    return "1";
L_40014:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "grace") == 0)
        goto L_403ec;
L_4003c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "laden") != 0)
        goto L_400c4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "bin") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "ibn") == 0)
        return "1";
L_400c4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "lead") != 0)
        goto L_40390;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") == 0)
        goto L_4035c;
L_4011c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_acetate))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "turn") == 0)
        goto L_40308;
L_40180:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0x103, 0), "name"), "alloy") != 0)
        goto L_401e0;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "of") == 0)
        goto L_40440;
L_401e0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_level))
        goto L_40248;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") == 0)
        return "1";
L_40248:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_level))
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_get))
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "the") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "out") != 0)
        goto L_3e1b4;
    return "1";
L_40308:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "into") != 0)
        goto L_40180;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "gold") != 0)
        goto L_40180;
    return "1";
L_4035c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_actor))
        goto L_4011c;
    return "1";
L_40390:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "lead's") != 0)
        goto L_4046c;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_chemical))
        goto L_3e1b4;
    return "1";
L_403ec:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "de") != 0)
        goto L_4003c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "coup") != 0)
        goto L_4003c;
    return "1";
L_40440:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "and") != 0)
        goto L_401e0;
    return "1";
L_4046c:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "louis") == 0)
        goto L_40524;
L_40490:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "mayer") != 0)
        goto L_40550;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_john))
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_marissa))
        return "0";
    return "n";
L_40524:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "saint") != 0)
        goto L_40490;
    return "1";
L_40550:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "mobile") != 0)
        goto L_405d4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "alabama") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "county") != 0)
        goto L_3e1b4;
    return "1";
L_405d4:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "overall") != 0)
        goto L_40630;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "gpos"), "content") == 0)
        return "n";
    return "a";
L_40630:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "regina") != 0)
        goto L_4067c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "spektor") != 0)
        goto L_407a0;
L_4067c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "row") != 0)
        goto L_40854;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "of") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "broke") != 0)
        goto L_40824;
L_40704:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "out") == 0)
        return "1";
L_40734:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "a") == 0)
        goto L_407ec;
L_40768:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_bitter))
        goto L_3e1b4;
    return "1";
L_407a0:
    if (phrase_contains_word(item_parent(it_r4), en_wl_canada))
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_in))
        goto L_3e1b4;
    return "1";
L_407ec:
    if (!cst_regex_match(en_rx_have, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        goto L_40768;
    return "1";
L_40824:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "breaks") == 0)
        goto L_40704;
    goto L_40734;
L_40854:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "sade") != 0)
        goto L_40c30;
L_40878:
    s7 = item_feat_string(item_parent(item_as(it_r4, 1)), "name");
    s_r6 = s7;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "de") == 0)
        return "1";
    if (!strstr(s_r6, "SADE"))
        goto L_41060;
    return "2";
L_40c30:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "sade's") == 0)
        goto L_40878;
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "sake") != 0)
        goto L_40fe8;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_bomb_2))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_japanese))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_brewing))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") == 0)
        goto L_40fc0;
L_40d2c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0x103, 0), "name"), en_wl_brewing))
        goto L_40d98;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "a") == 0)
        goto L_41090;
L_40d98:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_brewing))
        goto L_40e00;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_the))
        return "1";
L_40e00:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0x103, 0), "name"), en_wl_brewing))
        goto L_40e6c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "much") == 0)
        goto L_40f40;
L_40e6c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), en_wl_cup))
        goto L_40ed4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "of") == 0)
        goto L_40f94;
L_40ed4:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_cup))
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") != 0)
        goto L_3e1b4;
    return "1";
L_40f40:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "of") != 0)
        goto L_40e6c;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_the))
        goto L_40e6c;
    return "1";
L_40f94:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_the))
        goto L_40ed4;
    return "1";
L_40fc0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_brewing))
        goto L_40d2c;
    return "1";
L_40fe8:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "seat") != 0)
        goto L_410e4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "SEAT") != 0)
        goto L_3e1b4;
    if (!phrase_contains_word(item_parent(it_r4), en_wl_automobile))
        goto L_3e1b4;
    return "1";
L_41060:
    if (!phrase_contains_word(item_parent(it_r4), en_wl_alef))
        return "n";
    return "3";
L_41090:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "lot") != 0)
        goto L_40d98;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") != 0)
        goto L_40d98;
    return "1";
L_410e4:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "seat's") != 0)
        goto L_4115c;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "SEAT's") != 0)
        goto L_3e1b4;
    if (!phrase_contains_word(item_parent(it_r4), en_wl_automobile))
        goto L_3e1b4;
    return "1";
L_4115c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "shea") != 0)
        goto L_411ac;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "tree") != 0)
        goto L_3e1b4;
    return "1";
L_411ac:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "tori") != 0)
        goto L_4123c;
    s8 = item_feat_string(item_parent(item_as(it_r0, 1)), "name");
    s_r5 = s8;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_alamaze))
        return "1";
    if (strstr(s_r5, "Tori"))
        return "1";
    goto L_3e1b4;
L_4123c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "weiss") != 0)
        goto L_41290;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), en_wl_armin))
        goto L_3e1b4;
    return "1";
L_41290:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "wicked") == 0)
        return "a";
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "vide") != 0)
        goto L_41318;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "sous") != 0)
        goto L_3e1b4;
    return "1";
L_41318:
    it_r0 = it_r4;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0), "name"), en_wl_resign))
        goto L_414fc;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "gpos"), "pps$") != 0)
        goto L_4162c;
L_41374:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "new") != 0)
        goto L_415d0;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "contract") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "contracts") == 0)
        return "1";
L_41400:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_me_2))
        goto L_4153c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "0") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "gpos"), "in") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "with") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "to") == 0)
        return "1";
    return "v";
L_414fc:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "because") != 0)
        goto L_41598;
    if (item_next(it_r0))
        return "1";
    return "p";
L_4153c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "with") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "to") == 0)
        return "1";
    return "v";
L_41598:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "but") != 0)
        goto L_4165c;
    if (item_next(it_r0))
        goto L_3e1b4;
    return "1";
L_415d0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "contract") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "contracts") != 0)
        goto L_41400;
    return "1";
L_4162c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_the_2))
        goto L_41374;
    goto L_41400;
L_4165c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "to") != 0)
        goto L_417d4;
    it_r0 = it_r4;
    if (!item_next(it_r0))
        goto L_41740;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_is))
        goto L_41708;
    n9 = ((const signed char *)(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name")))[0];
    n_r3 = n9;
    if (n9 == -1)
        goto L_41d84;
    if (!(((const unsigned char *)((_ctype_ + (n_r3 & 255))))[1] & 1))
        goto L_41d84;
L_41708:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") != 0)
        goto L_3e1b4;
    return "2";
L_41740:
    if (!item_prev(it_r0))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") == 0)
        return "2";
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") != 0)
        return "1";
    return "2";
L_417d4:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "and") != 0)
        goto L_419cc;
L_417f8:
    if (!item_prev(it_r4))
        goto L_4193c;
L_41808:
    if (!item_prev(it_r4))
        goto L_41828;
    if (item_next(it_r4))
        goto L_3e1b4;
L_41828:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "and") != 0)
        goto L_419a0;
L_41858:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") == 0)
        return "2";
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") == 0)
        return "2";
L_418d4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "them") != 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") != 0)
        goto L_41950;
    return "0";
L_4193c:
    if (item_next(it_r4))
        goto L_41808;
    return "1";
L_41950:
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "0";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") == 0)
        return "0";
    return "1";
L_419a0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "our") == 0)
        goto L_41858;
    goto L_418d4;
L_419cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "our") == 0)
        goto L_417f8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "them") == 0)
        goto L_417f8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "for") != 0)
        goto L_41c28;
L_41a44:
    if (!item_next(it_r4))
        goto L_41b6c;
L_41a54:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "of") != 0)
        goto L_41aa0;
    if (!item_prev(it_r4))
        goto L_41e04;
    if (!item_next(it_r4))
        goto L_41e04;
L_41aa0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "from") != 0)
        goto L_3e1b4;
    if (!item_prev(it_r4))
        goto L_41aec;
    if (item_next(it_r4))
        goto L_3e1b4;
L_41aec:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") == 0)
        return "1";
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") != 0)
        goto L_3e1b4;
    return "1";
L_41b6c:
    if (!item_prev(it_r4))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "for") != 0)
        goto L_41a54;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") == 0)
        return "2";
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") != 0)
        goto L_41a54;
    return "2";
L_41c28:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "of") == 0)
        goto L_41a44;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "from") == 0)
        goto L_41a44;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "y") != 0)
        goto L_41f60;
    s10 = ffeature_string(item_traverse(it_r4, 0x103, 1, 0x106, 0), "name");
    s11 = ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name");
    s_Sc = s11;
    s_r9 = s10;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), (const char *const []){ "brut", "pobol", "craig", "ortega", "gymraeg", "gramadeg", "castell", "pen", "englynion", NULL }))
        goto L_41e84;
    return "1";
L_41d84:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") == 0)
        return "2";
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") == 0)
        return "2";
    return "1";
L_41e04:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-AU") == 0)
        return "2";
    if (cst_startswith(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-GB"))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "language_code"), "en-IN") != 0)
        goto L_41aa0;
    return "2";
L_41e84:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), (const char *const []){ "fenni", "brenhinedd", "gaer", "gododdin", NULL }))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), (const char *const []){ "suffolk", "super", "big", "street", "generation", "the", NULL }))
        return "n";
    n_r3 = ((const signed char *)(s_r9))[0];
    if (((const signed char *)(s_r9))[0] == -1)
        goto L_3e1b4;
    s_r3 = _ctype_;
    if (!(((const unsigned char *)((_ctype_ + (n_r3 & 255))))[1] & 1))
        goto L_3e1b4;
    n_r2 = ((const signed char *)(s_Sc))[0];
    if (((const signed char *)(s_Sc))[0] == -1)
        goto L_3e1b4;
    if (!(((const unsigned char *)((s_r3 + (n_r2 & 255))))[1] & 1))
        goto L_3e1b4;
    rx_r5 = cst_rx_lowercase;
    if (!cst_regex_match(cst_rx_lowercase, (s_r9 + 1)))
        goto L_3e1b4;
    if (!cst_regex_match(rx_r5, (s_Sc + 1)))
        goto L_3e1b4;
    return "c";
L_41f60:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "nice") != 0)
        goto L_42008;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "Nice") == 0)
        goto L_41fcc;
    return "a";
L_41fcc:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_france))
        goto L_42060;
    return "n";
L_42008:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "sabine") != 0)
        goto L_42118;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_king))
        goto L_3e1b4;
    return "1";
L_42060:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_in_2))
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_42208;
L_420b8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        return "a";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "france") == 0)
        return "n";
    return "a";
L_42118:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "scone") != 0)
        goto L_422b8;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "Scone") != 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_abbey))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") == 0)
        goto L_4228c;
L_421c0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_horse))
        goto L_42238;
    return "2";
L_42208:
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "france") != 0)
        goto L_420b8;
    return "n";
L_42238:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "australia") == 0)
        return "2";
    return "n";
L_4228c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "scotland") != 0)
        goto L_421c0;
    return "1";
L_422b8:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "negros") != 0)
        goto L_42364;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "Negros") != 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_occidental))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_in_3))
        return "1";
    return "n";
L_42364:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "nevis") != 0)
        goto L_423b4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "ben") != 0)
        goto L_3e1b4;
    return "1";
L_423b4:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "lima") != 0)
        goto L_42724;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "Lima") != 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "peru") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") == 0)
        goto L_426f8;
L_42464:
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x103, 0), "name"), "capital") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x104, 0), "name"), "capital") == 0)
        goto L_42f68;
L_424cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "capital") != 0)
        goto L_42528;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "is") == 0)
        return "1";
L_42528:
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x105, 0), "name"), "capital") != 0)
        goto L_4258c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x104, 0), "name"), "city") == 0)
        return "1";
L_4258c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "is") != 0)
        goto L_425e8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "the") == 0)
        goto L_42f98;
L_425e8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "of") != 0)
        goto L_42644;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "peru") == 0)
        goto L_426cc;
L_42644:
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "the") != 0)
        goto L_426a8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x101, 0), "name"), "capital") == 0)
        return "1";
L_426a8:
    if (item_prev(it_r4))
        return "n";
    if (item_next(it_r4))
        return "n";
    return "1";
L_426cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "is") != 0)
        goto L_42644;
    return "1";
L_426f8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "peru") != 0)
        goto L_42464;
    return "1";
L_42724:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "louisville") != 0)
        goto L_429ac;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "kentucky") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name"), "KY") == 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_colorado))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name"), en_wl_co))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "new") == 0)
        goto L_42980;
L_4283c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_42928;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "kentucky") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 1, 0x106, 0), "name"), "KY") == 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_colorado))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "new") == 0)
        goto L_42a08;
L_42928:
    if (phrase_contains_word(item_parent(it_r4), en_wl_kentucky))
        goto L_3e1b4;
    if (!phrase_contains_word(item_parent(it_r4), en_wl_colorado))
        goto L_3e1b4;
    if (!phrase_contains_word(item_parent(it_r4), en_wl_boulder))
        goto L_3e1b4;
    return "1";
L_42980:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "york") != 0)
        goto L_4283c;
    return "1";
L_429ac:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "abel") != 0)
        goto L_42a34;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_tesfaye))
        goto L_3e1b4;
    return "1";
L_42a08:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "york") != 0)
        goto L_42928;
    return "1";
L_42a34:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "beaufort") != 0)
        goto L_43070;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "south") == 0)
        goto L_43044;
L_42a8c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name"), "SC") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") == 0)
        goto L_42ff0;
L_42af8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "county") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "south") == 0)
        goto L_42fc4;
L_42b54:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 1, 0x106, 0), "name"), "SC") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "in") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "south") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0x100, 0), "name"), "carolina") != 0)
        goto L_3e1b4;
    return "1";
L_42f68:
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x103, 0), "name"), "city") != 0)
        goto L_424cc;
    return "1";
L_42f98:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "capital") != 0)
        goto L_425e8;
    return "1";
L_42fc4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "carolina") != 0)
        goto L_42b54;
    return "1";
L_42ff0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "south") != 0)
        goto L_42af8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "carolina") != 0)
        goto L_42af8;
    return "1";
L_43044:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "carolina") != 0)
        goto L_42a8c;
    return "1";
L_43070:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "birmingham") != 0)
        goto L_431d0;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_england))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_431a4;
L_430f8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_4315c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_england))
        return "1";
L_4315c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_alabama))
        goto L_43538;
    return "2";
L_431a4:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_england))
        goto L_430f8;
    return "1";
L_431d0:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "elgin") != 0)
        goto L_43424;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_moray))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_435f4;
L_43258:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_432bc;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_moray))
        return "1";
L_432bc:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_il))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_433f8;
L_43328:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_43388;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_il))
        return "2";
L_43388:
    if (!phrase_contains_word(item_parent(it_r4), en_wl_bastrop))
        goto L_433c8;
    if (!phrase_contains_word(item_parent(it_r4), en_wl_cook))
        return "1";
L_433c8:
    if (phrase_contains_word(item_parent(it_r4), en_wl_cook))
        goto L_43620;
    return "n";
L_433f8:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_il))
        goto L_43328;
    return "2";
L_43424:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "harwich") != 0)
        goto L_43640;
    if (strcmp(ffeature_string(item_traverse(it_r0, 2, 0x100, 0), "name"), "essex") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") == 0)
        goto L_435cc;
L_434a8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "massachusetts") == 0)
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "massachusetts") != 0)
        goto L_3e1b4;
    return "2";
L_43538:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_43938;
L_43568:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_alabama))
        return "2";
    return "n";
L_435cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "essex") != 0)
        goto L_434a8;
    return "1";
L_435f4:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_moray))
        goto L_43258;
    return "1";
L_43620:
    if (!phrase_contains_word(item_parent(it_r4), en_wl_bastrop))
        return "2";
    return "n";
L_43640:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "lancaster") != 0)
        goto L_43860;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_england_2))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_437d8;
L_436c8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_4372c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_england_2))
        return "1";
L_4372c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_penn))
        return "2";
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "punc"), ",") == 0)
        goto L_43834;
L_43798:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") == 0)
        goto L_43804;
    return "n";
L_437d8:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_england_2))
        goto L_436c8;
    return "1";
L_43804:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_penn))
        return "2";
    return "n";
L_43834:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_penn))
        goto L_43798;
    return "2";
L_43860:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "barre") != 0)
        goto L_43964;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "Barre") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "vermont") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "vermont") != 0)
        goto L_3e1b4;
    return "1";
L_43938:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_alabama))
        goto L_43568;
    return "2";
L_43964:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "lyon") != 0)
        goto L_43a10;
    if (strcmp(ffeature_string(item_traverse(it_r0, 2, 0x100, 0), "name"), "france") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "france") != 0)
        goto L_3e1b4;
    return "1";
L_43a10:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "carmi") != 0)
        goto L_43aec;
    if (strcmp(ffeature_string(item_traverse(it_r0, 1, 0x106, 0), "name"), "Carmi") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "illinois") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "illinois") != 0)
        goto L_3e1b4;
    return "1";
L_43aec:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "reading") != 0)
        goto L_43dec;
    v12 = item_feat(item_parent(item_as(it_r0, 1)), "interpret-as");
    v_r0 = v12;
    if (!v12)
        goto L_43b4c;
    if (strcmp(val_string(v_r0), "address") == 0)
        return "1";
L_43b4c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Reading") != 0)
        goto L_3e1b4;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), en_wl_berkshire))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 2, 0x100, 0), "name"), "new") == 0)
        goto L_43d64;
L_43be4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_43c70;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_berkshire))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "new") == 0)
        goto L_43e4c;
L_43c70:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), en_wl_borough))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x102, 0x100, 0), "name"), en_wl_borough))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "and") == 0)
        goto L_43dc0;
L_43cfc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "festival") != 0)
        goto L_3e1b4;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_florida))
        goto L_3e1b4;
    return "1";
L_43d64:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "york") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "jersey") != 0)
        goto L_43be4;
    return "1";
L_43dc0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "leeds") != 0)
        goto L_43cfc;
    return "1";
L_43dec:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "grande") != 0)
        goto L_43ea8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "ariana") != 0)
        goto L_3e1b4;
    return "n";
L_43e4c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "york") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x102, 0), "name"), "jersey") != 0)
        goto L_43c70;
    return "1";
L_43ea8:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "jehan") != 0)
        goto L_43f0c;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_adam))
        goto L_3e1b4;
    return "1";
L_43f0c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "nuh") != 0)
        goto L_43f64;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_uh))
        goto L_3e1b4;
    return "1";
L_43f64:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "ruh") != 0)
        goto L_43fbc;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_roh))
        goto L_3e1b4;
    return "1";
L_43fbc:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "weiner") != 0)
        goto L_4404c;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "anthony") == 0)
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_ehud))
        goto L_3e1b4;
    return "2";
L_4404c:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "real") != 0)
        goto L_44140;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), "camino") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "sociedad") == 0)
        return "1";
    n13 = ((const signed char *)(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name")))[0];
    n_r3 = n13;
    if (n13 == -1)
        goto L_3e1b4;
    if (!(((const unsigned char *)((_ctype_ + (n_r3 & 255))))[1] & 1))
        goto L_3e1b4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "madrid") != 0)
        goto L_3e1b4;
    return "1";
L_44140:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "lech") != 0)
        goto L_441c0;
    if (cst_member_string(ffeature_string(item_traverse(it_r0, 0x103, 0), "name"), en_wl_andrzej))
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_aleksander))
        goto L_3e1b4;
    return "1";
L_441c0:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "les") != 0)
        goto L_44214;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_baxter))
        goto L_3e1b4;
    return "1";
L_44214:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "angela") != 0)
        goto L_44264;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), "merkel") != 0)
        goto L_3e1b4;
    return "1";
L_44264:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "werner") != 0)
        goto L_442bc;
    if (!cst_member_string(ffeature_string(item_traverse(it_r0, 0x100, 0), "name"), en_wl_vogels))
        goto L_3e1b4;
    return "1";
L_442bc:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "denier") == 0)
        goto L_443a8;
L_442e0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "offset") != 0)
        goto L_44338;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "aux") == 0)
        return "v";
L_44338:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "wound") != 0)
        goto L_444b4;
L_44368:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "up") != 0)
        goto L_443f0;
    return "t";
L_443a8:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_zero))
        goto L_442e0;
    return "1";
L_443f0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "down") == 0)
        return "t";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "wind") == 0)
        return "t";
L_44448:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "number") != 0)
        goto L_445d8;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_zero))
        goto L_444e0;
    return "n";
L_444b4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "wounds") == 0)
        goto L_44368;
    goto L_44448;
L_444e0:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_and_2))
        goto L_448e8;
L_44504:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_get_2))
        return "a";
L_445d8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "perfect") != 0)
        goto L_44698;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") == 0)
        goto L_47acc;
L_44630:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "it") == 0)
        return "a";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_me_2))
        goto L_47a94;
L_44698:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "live") == 0)
        goto L_47b84;
L_446c8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "minute") != 0)
        goto L_44964;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "one") == 0)
        goto L_47b28;
L_44720:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "this") != 0)
        goto L_4477c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "very") == 0)
        return "n";
L_4477c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "the") != 0)
        goto L_447d8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "only") == 0)
        return "n";
L_447d8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0x103, 0), "name"), "the") != 0)
        goto L_44838;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "and") == 0)
        goto L_47c34;
L_44838:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "by") != 0)
        goto L_47af8;
L_4486c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "minute") == 0)
        return "n";
L_4489c:
    if (!cst_regex_match(en_rx_be, ffeature_string(item_traverse(it_r4, 0x103, 0), "name")))
        goto L_44914;
    return "a";
L_448e8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "0") == 0)
        goto L_44504;
    goto L_445d8;
L_44914:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        return "a";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "away") == 0)
        return "n";
L_44964:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "route") != 0)
        goto L_44cc0;
L_44994:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), (const char *const []){ "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety", NULL }))
        goto L_44c84;
L_449e0:
    it_r0 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "street") != 0)
        goto L_44a8c;
    it14 = item_parent(item_as(it_r0, 1));
    s15 = item_feat_string(it14, "name");
    v16 = item_feat(it14, "interpret-as");
    s_r5 = s15;
    v_r0 = v16;
    if (!v16)
        goto L_44a60;
    if (strcmp(val_string(v_r0), "address") == 0)
        return "1";
L_44a60:
    if (strcmp(s_r5, "Street") != 0)
        return "n";
    if (item_prev(it_r4))
        return "1";
    return "n";
L_44a8c:
    if (strcmp(ffeature_string(item_traverse(it_r0, 0), "name"), "bow") == 0)
        goto L_44c58;
L_44ab0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "bow") != 0)
        goto L_44c2c;
L_44ae0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "in") != 0)
        goto L_44b3c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "the") == 0)
        return "1";
L_44b3c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "wow") == 0)
        return "1";
L_44b9c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "sine") != 0)
        goto L_45324;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "qua") != 0)
        goto L_44f34;
L_44bf8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "non") == 0)
        return "1";
    return "n";
L_44c2c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "bows") == 0)
        goto L_44ae0;
    goto L_44b9c;
L_44c58:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "by") != 0)
        goto L_44ab0;
    return "1";
L_44c84:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_cable))
        goto L_44cec;
    return "v";
L_44cc0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "routes") == 0)
        goto L_44994;
    goto L_449e0;
L_44cec:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), "of") == 0)
        return "v";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_i))
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "md") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "md") == 0)
        goto L_47cf0;
L_44da0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "who") == 0)
        return "v";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_do))
        return "v";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_do))
        goto L_47cc0;
L_44e30:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_me))
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "det") != 0)
        goto L_47c90;
L_44e98:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "det") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "md") == 0)
        goto L_47c60;
L_44ef8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "pps$") != 0)
        return "1";
    return "v";
L_44f34:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "que") == 0)
        goto L_44bf8;
    return "n";
L_45324:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "axes") == 0)
        goto L_47db4;
L_4534c:
    s17 = cst_strrchr(item_feat_string(it_r4, "name"), 39);
    s_r0 = s17;
    if (!s17)
        goto L_45418;
    if (strcmp(s_r0, "'s") != 0)
        goto L_45418;
L_45380:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "polish") != 0)
        goto L_453dc;
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Polish") == 0)
        return "a";
L_453dc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "_a") != 0)
        goto L_47d28;
    return "0";
L_45418:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "pps$") == 0)
        goto L_48074;
L_4544c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_the_2))
        goto L_45380;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "per") == 0)
        goto L_45380;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "det") == 0)
        goto L_4803c;
L_454e4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") == 0)
        goto L_45380;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "of") == 0)
        goto L_45648;
L_45544:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "of") == 0)
        goto L_45380;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_listen))
        goto L_455d8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") == 0)
        goto L_45380;
L_455d8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "wind") != 0)
        goto L_458d4;
L_45608:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "up") != 0)
        goto L_45674;
    return "v";
L_45648:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "no") == 0)
        goto L_45380;
    goto L_45544;
L_45674:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "down") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "get") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "got") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "farm") == 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_road))
        return "v";
L_45758:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "record") != 0)
        goto L_48010;
L_45788:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "world") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "video") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "videos") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "high") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "highs") == 0)
        return "n";
L_4586c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "di") != 0)
        goto L_4592c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "lady") != 0)
        goto L_45900;
    return "1";
L_458d4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "winds") == 0)
        goto L_45608;
    goto L_45758;
L_45900:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "princess") == 0)
        return "1";
L_4592c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "do") != 0)
        goto L_47fe4;
L_4595c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_do_3))
        goto L_48138;
L_4599c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_mi))
        goto L_45a2c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_do_3))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_do_3))
        return "1";
L_45a2c:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_mi))
        goto L_45abc;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_do_3))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_do_3))
        return "1";
L_45abc:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_mi))
        goto L_45b24;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_do_3))
        return "1";
L_45b24:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_mi))
        goto L_45b8c;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_do_3))
        return "1";
L_45b8c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Do") != 0)
        goto L_480a0;
L_45bc4:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 1, 0x106, 0), "name"), en_wl_do_4))
        goto L_45c70;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 1, 0x106, 0), "name"), en_wl_do_4))
        return "1";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name"), en_wl_do_4))
        return "1";
L_45c70:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 1, 0x106, 0), "name"), en_wl_do_4))
        goto L_480d4;
L_45cbc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "dies") != 0)
        goto L_45d18;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_amoris))
        return "1";
L_45d18:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "use") != 0)
        goto L_45e0c;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), en_wl_stay))
        goto L_46168;
L_45d74:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "to") != 0)
        goto L_45dd8;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_good))
        return "n";
L_45dd8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), "put") == 0)
        goto L_46114;
L_45e0c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "tov") != 0)
        goto L_45ec0;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "mazel") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "mazal") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "mahzel") == 0)
        return "1";
L_45ec0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "poke") != 0)
        goto L_45f20;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_ahi))
        return "1";
L_45f20:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "project") != 0)
        goto L_46030;
L_45f50:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Project") == 0)
        return "n";
L_45fec:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_460ec;
    return "v";
L_46030:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "projects") == 0)
        goto L_45f50;
L_46058:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "of") != 0)
        goto L_461f4;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_461f4;
L_460b4:
    if (en_lex_select_pos(it_r4, item_feat_string(it_r4, "name"), "a") > 0)
        return "a";
    return "n";
L_460ec:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_accidentally))
        goto L_46058;
    return "v";
L_46114:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "it") != 0)
        goto L_45e0c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") != 0)
        goto L_45e0c;
    return "n";
L_46168:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "put") != 0)
        goto L_45d74;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") != 0)
        goto L_45d74;
    return "n";
L_461bc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 1, 0x106, 0), "name"), "IT") != 0)
        goto L_45fec;
    return "n";
L_461f4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "pps") != 0)
        goto L_4625c;
    if (cst_regex_match(en_rx_be, ffeature_string(item_traverse(it_r4, 0x103, 0), "name")))
        goto L_460b4;
L_4625c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") != 0)
        goto L_463b0;
L_46290:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") != 0)
        goto L_462e4;
    if (cst_endswith(item_feat_string(it_r4, "name"), "s"))
        goto L_481ec;
L_462e4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "that") != 0)
        goto L_46344;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_me))
        return "n";
L_46344:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "pps$") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "more") == 0)
        return "n";
    return "v";
L_463b0:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_i))
        goto L_46290;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "md") == 0)
        goto L_46290;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "md") == 0)
        goto L_481c0;
L_46440:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "who") == 0)
        goto L_46290;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_do))
        goto L_46290;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_do))
        goto L_48190;
L_464d0:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_me))
        goto L_46290;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "det") != 0)
        goto L_48160;
L_46538:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "det") == 0)
        goto L_46290;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "md") == 0)
        goto L_4823c;
L_46598:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "pps$") == 0)
        goto L_46290;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "use") != 0)
        goto L_48210;
L_465f8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "in") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "in") == 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x105, 0), "name"), en_wl_have))
        goto L_4826c;
L_46694:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_have))
        goto L_466fc;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") != 0)
        return "n";
L_466fc:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_have))
        return "n";
L_46734:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "live") != 0)
        goto L_469c0;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_music))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_music))
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") == 0)
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_sold))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_announced))
        return "n";
    if (!item_prev(it_r4))
        goto L_47a50;
L_46844:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "from") != 0)
        goto L_468a8;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_i_2))
        goto L_4829c;
L_468a8:
    if (!item_next(it_r4))
        goto L_479b0;
L_468b8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Live") != 0)
        goto L_46970;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "in") != 0)
        goto L_47a64;
L_4691c:
    n18 = ((const signed char *)(ffeature_string(item_traverse(it_r4, 0x101, 1, 0x106, 0), "name")))[0];
    n_r3 = n18;
    if (n18 == -1)
        goto L_46970;
    if ((((const unsigned char *)((_ctype_ + (n_r3 & 255))))[1] & 1))
        return "n";
L_46970:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_afore))
        return "n";
    if (!item_next(item_as(it_r4, 1)))
        goto L_46aa8;
L_469c0:
    n19 = strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "lives");
    n_Sc = n19;
    if (n19 != 0)
        goto L_46c2c;
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, n_Sc, 0), "name"), en_wl_the_2))
        goto L_46b2c;
    return "v";
L_46aa8:
    s20 = ffeature_string(item_traverse(it_r4, 1, 0x106, 0x100, 0), "name");
    s_r6 = s20;
    if (!cst_regex_match(cst_rx_digits, s20))
        goto L_469c0;
    if (strlen(s_r6) != 4)
        goto L_469c0;
    if (cst_startswith(s_r6, "19"))
        return "n";
    if (!cst_startswith(s_r6, "20"))
        goto L_469c0;
    return "n";
L_46b2c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "there") != 0)
        goto L_46b98;
    if (!cst_regex_match(en_rx_be, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        goto L_47a18;
    return "n";
L_46b98:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), (const char *const []){ "their", "the", "our", "own", NULL }))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), (const char *const []){ "of", "are", "were", "have", "than", "would", "will", NULL }))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), (const char *const []){ "who", "he", "she", NULL }))
        return "v";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), (const char *const []){ "in", "on", "with", "there", "here", "up", "near", "alone", "next", "within", "under", NULL }))
        return "v";
L_46c2c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "elaborate") == 0)
        goto L_4710c;
L_46c5c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "resume") != 0)
        goto L_46cbc;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_free))
        return "n";
L_46cbc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "dove") != 0)
        goto L_46d50;
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_the_2))
        goto L_47170;
L_46d20:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "in") == 0)
        return "v";
L_46d50:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "conflicts") != 0)
        goto L_46da8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "with") == 0)
        return "v";
L_46da8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "produce") != 0)
        goto L_46f60;
    if (cst_endswith(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "'s"))
        return "n";
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_harvested))
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "in") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "cc") != 0)
        goto L_471cc;
L_46e90:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 0), "name"), en_wl_fruit))
        return "n";
L_46ec8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "gpos"), "cc") != 0)
        goto L_470d8;
L_46ef8:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_fruit))
        return "n";
L_46f30:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "executive") == 0)
        return "n";
L_46f60:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "affect") != 0)
        goto L_47024;
    rx_r5 = en_rx_have;
    if (cst_regex_match(en_rx_have, ffeature_string(item_traverse(it_r4, 0x103, 0), "name")))
        return "n";
    if (cst_regex_match(rx_r5, ffeature_string(item_traverse(it_r4, 0x104, 0), "name")))
        goto L_47140;
L_46ff0:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_overall))
        return "n";
L_47024:
    if (cst_endswith(item_feat_string(it_r4, "name"), "s'"))
        return "n";
    p_r4 = it_r4;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "refuse") == 0)
        goto L_3f080;
L_47070:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0), "name"), "polish") != 0)
        goto L_474f8;
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "zloty") != 0)
        goto L_47728;
    return "a";
L_470d8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") == 0)
        goto L_46ef8;
    goto L_46f30;
L_4710c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "on") != 0)
        goto L_46c5c;
    return "v";
L_47140:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "some") != 0)
        goto L_46ff0;
    return "n";
L_47170:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "rock") != 0)
        goto L_47914;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "in") == 0)
        return "v";
    return "n";
L_471cc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "to") == 0)
        goto L_46e90;
    goto L_46ec8;
L_474f8:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0), "name"), "buffet") == 0)
        goto L_476fc;
L_47528:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0), "name"), "buffets") != 0)
        goto L_47580;
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x103, 0), "name"), "wind") == 0)
        return "v";
L_47580:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0), "name"), "close") != 0)
        goto L_475e0;
    if (cst_member_string(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), en_wl_window))
        return "v";
L_475e0:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0), "name"), "inter") != 0)
        goto L_47638;
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "milan") == 0)
        return "p";
L_47638:
    it_r4 = p_r4;
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x103, 0), "gpos"), "in") != 0)
        goto L_3e1b4;
    it_r4 = p_r4;
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "gpos"), "content") != 0)
        goto L_3e1b4;
    it_r4 = p_r4;
    if (en_lex_select_pos(p_r4, item_feat_string(p_r4, "name"), "a") <= 0)
        goto L_3e1b4;
    it_r4 = p_r4;
    if (en_lex_select_pos(p_r4, item_feat_string(p_r4, "name"), "n") > 0)
        goto L_3e1b4;
    return "a";
L_476fc:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x103, 0), "name"), "winds") != 0)
        goto L_47528;
    return "v";
L_47728:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "zlotys") == 0)
        return "a";
    if (strcmp(ffeature_string(item_traverse(p_r4, 1, 0x106, 0), "name"), "Polish") != 0)
        goto L_474f8;
    if (cst_member_string(ffeature_string(item_traverse(p_r4, 0x103, 0), "name"), en_wl_body))
        return "n";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "remover") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "removers") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "up") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "off") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "out") == 0)
        return "v";
    if (!item_prev(p_r4))
        goto L_4794c;
L_478a0:
    if (item_prev(p_r4))
        return "a";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "by") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x101, 0), "name"), "by") == 0)
        return "v";
    return "a";
L_47914:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_beautiful))
        goto L_46d20;
    return "n";
L_4794c:
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x100, 0), "name"), "with") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(p_r4, 0x101, 0), "name"), "with") != 0)
        goto L_478a0;
    return "v";
L_479b0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") == 0)
        goto L_468b8;
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Live") != 0)
        goto L_468b8;
    return "n";
L_47a18:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "gpos"), "md") != 0)
        return "v";
    return "n";
L_47a50:
    if (item_next(it_r4))
        goto L_46844;
    return "n";
L_47a64:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "at") == 0)
        goto L_4691c;
    goto L_46970;
L_47a94:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), en_wl_help))
        return "v";
    return "a";
L_47acc:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "close") != 0)
        goto L_44630;
    return "a";
L_47af8:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "to") == 0)
        goto L_4486c;
    goto L_4489c;
L_47b28:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "more") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "less") != 0)
        goto L_44720;
    return "n";
L_47b84:
    if (cst_endswith(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "radio"))
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "tour") == 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "forever") == 0)
        return "v";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), "aid") != 0)
        goto L_446c8;
    return "n";
L_47c34:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "only") != 0)
        goto L_44838;
    return "n";
L_47c60:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_44ef8;
    return "v";
L_47c90:
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_the_2))
        goto L_44e98;
    return "v";
L_47cc0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "not") != 0)
        goto L_44e30;
    return "v";
L_47cf0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "not") != 0)
        goto L_44da0;
    return "v";
L_47d28:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "content") != 0)
        return "n";
    if (en_lex_select_pos(it_r4, item_feat_string(it_r4, "name"), "a") <= 0)
        return "n";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "number") == 0)
        return "n";
    return "a";
L_47db4:
    if (phrase_contains_word(item_parent(it_r4), en_wl_orientation))
        return "1";
    it21 = item_parent(it_r4);
    it22 = phrase_start_token(it21, 0);
    it23 = item_next(phrase_end_token(it21, 0));
    it_r6 = item_as(item_daughter(it22), 2);
    it_r9 = it23;
L_47e30:
    it_r0 = it_r6;
    if (!it_r6)
        goto L_47ea0;
    it_r0 = it_r6;
    if (it_r9 == item_parent(item_as(it_r0, 3)))
        goto L_47ea0;
    it_r0 = it_r6;
    if (cst_startswith_any(ffeature_string(item_traverse(it_r0, 0), "name"), en_wl_ordinate))
        return "1";
    it_r0 = it_r6;
    if (cst_endswith_any(ffeature_string(item_traverse(it_r0, 0), "name"), en_wl_ordinate))
        return "1";
    it_r6 = item_next(it_r0);
    goto L_47e30;
L_47ea0:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_major))
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "along") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "along") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "on") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "on") == 0)
        return "1";
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "ratio") != 0)
        goto L_4534c;
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "of") != 0)
        goto L_4534c;
    return "1";
L_47fe4:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "re") == 0)
        goto L_4595c;
    goto L_45cbc;
L_48010:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "records") == 0)
        goto L_45788;
    goto L_4586c;
L_4803c:
    if (cst_regex_match(en_rx_be, ffeature_string(item_traverse(it_r4, 0x100, 0), "name")))
        goto L_45380;
    goto L_454e4;
L_48074:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x100, 0), "gpos"), "det") != 0)
        goto L_45380;
    goto L_4544c;
L_480a0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 1, 0x106, 0), "name"), "Re") == 0)
        goto L_45bc4;
    goto L_45cbc;
L_480d4:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x101, 1, 0x106, 0), "name"), en_wl_do_4))
        return "1";
    if (!cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 1, 0x106, 0), "name"), en_wl_do_4))
        goto L_45cbc;
    return "1";
L_48138:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_do_3))
        goto L_4599c;
    goto L_45cbc;
L_48160:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x100, 0), "name"), en_wl_the_2))
        goto L_46290;
    goto L_46538;
L_48190:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "not") == 0)
        goto L_46290;
    goto L_464d0;
L_481c0:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "not") == 0)
        goto L_46290;
    goto L_46440;
L_481ec:
    if (cst_endswith(item_feat_string(it_r4, "name"), "ss"))
        goto L_462e4;
    return "n";
L_48210:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0), "name"), "uses") == 0)
        goto L_465f8;
    goto L_46734;
L_4823c:
    if (cst_member_string(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), en_wl_accidentally))
        goto L_46290;
    goto L_46598;
L_4826c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x104, 0), "name"), "to") == 0)
        goto L_46694;
    return "n";
L_4829c:
    if (strcmp(ffeature_string(item_traverse(it_r4, 0x103, 0), "name"), "to") == 0)
        goto L_468a8;
    return "n";
}
