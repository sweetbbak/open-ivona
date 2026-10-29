/*
 * English number expansion (0x4d0b8..; IVONA's version of Flite's us_expand.c): numbers,
 * digits, ordinals, years and ids, reals, money, times, UTC offsets and dates; every function
 * takes the token for the accent (British English says "one hundred and five").
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_ffeatures.h"
#include "cst_item.h"
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_val.h"

cst_item *item_traverse(const cst_item *item, ...);
cst_val *cst_split_tokens_string(const char *string);
int cst_startswith_any(const char *s, const char *const *slist);
int token_no_punc_prev(const cst_item *item);
int token_no_punc_next(const cst_item *item);
int en_is_dotted_abbrev(const cst_item *item, const char *s);
char *en_lowercase(const char *s);

typedef struct {
    char *day;
    char *month;
    char *year;
} cst_date;
void date_parse(cst_date *d, const char *str, const char *format, const cst_regex *month_regex,
                const void *month_table);
void date_free(cst_date *d);

extern const cst_regex *const en_rx_money;

/* a currency (en_exp_currency) */
typedef struct en_currency {
    const char *symbol;
    const char *unit, *units;
    const char *subunit, *subunits;
    const char *qualifier;          /* "australian" (dollars) */
    int flags;                      /* 1: qualifier after the unit; 2: not said with the unit */
} en_currency;

/* a parsed amount of money (en_parse_money) */
typedef struct en_money {
    int sign;                       /* '-', '+', 0xb1 or 0 */
    char *currency;
    char *amount;
    const char *scale;              /* "k", "M", "B", "T" (in the token) or NULL */
} en_money;

static const char *const ord2num[] = {
    "zeroth", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth"
};
static const char *const ord2teen[] = {
    "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth",
    "seventeenth", "eighteenth", "nineteenth"
};
static const char *const ord2enty[] = {
    "zeroth", "tenth", "twentieth", "thirtieth", "fortieth", "fiftieth", "sixtieth",
    "seventieth", "eightieth", "ninetieth"
};

/* 0x3d4d74: the name of each Latin-1 character, and another name (or NULL) */
static const char *const en_char_names[256][2] = {
    { NULL, NULL },  /* 0x00 */
    { NULL, NULL },  /* 0x01 */
    { NULL, NULL },  /* 0x02 */
    { NULL, NULL },  /* 0x03 */
    { NULL, NULL },  /* 0x04 */
    { NULL, NULL },  /* 0x05 */
    { NULL, NULL },  /* 0x06 */
    { NULL, NULL },  /* 0x07 */
    { "backspace", NULL },  /* 0x08 */
    { "tab", NULL },  /* 0x09 */
    { "line feed", NULL },  /* 0x0a */
    { "vertical tab", NULL },  /* 0x0b */
    { "form feed", NULL },  /* 0x0c */
    { "carriage return", NULL },  /* 0x0d */
    { NULL, NULL },  /* 0x0e */
    { NULL, NULL },  /* 0x0f */
    { NULL, NULL },  /* 0x10 */
    { NULL, NULL },  /* 0x11 */
    { NULL, NULL },  /* 0x12 */
    { NULL, NULL },  /* 0x13 */
    { NULL, NULL },  /* 0x14 */
    { NULL, NULL },  /* 0x15 */
    { NULL, NULL },  /* 0x16 */
    { NULL, NULL },  /* 0x17 */
    { NULL, NULL },  /* 0x18 */
    { NULL, NULL },  /* 0x19 */
    { NULL, NULL },  /* 0x1a */
    { "escape", NULL },  /* 0x1b */
    { NULL, NULL },  /* 0x1c */
    { NULL, NULL },  /* 0x1d */
    { NULL, NULL },  /* 0x1e */
    { NULL, NULL },  /* 0x1f */
    { "space", NULL },  /* 0x20 */
    { "exclamation mark", "exclamation point" },  /* 0x21 */
    { "quote", "double quote" },  /* 0x22 */
    { "hash", "number sign" },  /* 0x23 */
    { "dollar", "dollar sign" },  /* 0x24 */
    { "percent", "percent sign" },  /* 0x25 */
    { "and", "ampersand" },  /* 0x26 */
    { "apostrophe", NULL },  /* 0x27 */
    { "opening parenthesis", "left parenthesis" },  /* 0x28 */
    { "closing parenthesis", "right parenthesis" },  /* 0x29 */
    { "asterisk", NULL },  /* 0x2a */
    { "plus", "plus sign" },  /* 0x2b */
    { "comma", NULL },  /* 0x2c */
    { "dash", NULL },  /* 0x2d */
    { "dot", "period" },  /* 0x2e */
    { "slash", "forward slash" },  /* 0x2f */
    { "oh", "zero" },  /* 0x30 */
    { "one", NULL },  /* 0x31 */
    { "two", NULL },  /* 0x32 */
    { "three", NULL },  /* 0x33 */
    { "four", NULL },  /* 0x34 */
    { "five", NULL },  /* 0x35 */
    { "six", NULL },  /* 0x36 */
    { "seven", NULL },  /* 0x37 */
    { "eight", NULL },  /* 0x38 */
    { "nine", NULL },  /* 0x39 */
    { "colon", NULL },  /* 0x3a */
    { "semicolon", NULL },  /* 0x3b */
    { "less than", "less than sign" },  /* 0x3c */
    { "equals", "equals sign" },  /* 0x3d */
    { "greater than", "greater than sign" },  /* 0x3e */
    { "question mark", NULL },  /* 0x3f */
    { "at", "at sign" },  /* 0x40 */
    { "_a", "capital _a" },  /* 0x41 */
    { "b", "capital b" },  /* 0x42 */
    { "c", "capital c" },  /* 0x43 */
    { "d", "capital d" },  /* 0x44 */
    { "e", "capital e" },  /* 0x45 */
    { "f", "capital f" },  /* 0x46 */
    { "g", "capital g" },  /* 0x47 */
    { "h", "capital h" },  /* 0x48 */
    { "i", "capital i" },  /* 0x49 */
    { "j", "capital j" },  /* 0x4a */
    { "k", "capital k" },  /* 0x4b */
    { "l", "capital l" },  /* 0x4c */
    { "m", "capital m" },  /* 0x4d */
    { "n", "capital n" },  /* 0x4e */
    { "o", "capital o" },  /* 0x4f */
    { "p", "capital p" },  /* 0x50 */
    { "q", "capital q" },  /* 0x51 */
    { "r", "capital r" },  /* 0x52 */
    { "s", "capital s" },  /* 0x53 */
    { "t", "capital t" },  /* 0x54 */
    { "u", "capital u" },  /* 0x55 */
    { "v", "capital v" },  /* 0x56 */
    { "w", "capital w" },  /* 0x57 */
    { "x", "capital x" },  /* 0x58 */
    { "y", "capital y" },  /* 0x59 */
    { "z", "capital z" },  /* 0x5a */
    { "opening bracket", "left square bracket" },  /* 0x5b */
    { "backslash", NULL },  /* 0x5c */
    { "closing bracket", "right square bracket" },  /* 0x5d */
    { "caret", NULL },  /* 0x5e */
    { "underscore", NULL },  /* 0x5f */
    { "grave accent", NULL },  /* 0x60 */
    { "_a", "lowercase _a" },  /* 0x61 */
    { "b", "lowercase b" },  /* 0x62 */
    { "c", "lowercase c" },  /* 0x63 */
    { "d", "lowercase d" },  /* 0x64 */
    { "e", "lowercase e" },  /* 0x65 */
    { "f", "lowercase f" },  /* 0x66 */
    { "g", "lowercase g" },  /* 0x67 */
    { "h", "lowercase h" },  /* 0x68 */
    { "i", "lowercase i" },  /* 0x69 */
    { "j", "lowercase j" },  /* 0x6a */
    { "k", "lowercase k" },  /* 0x6b */
    { "l", "lowercase l" },  /* 0x6c */
    { "m", "lowercase m" },  /* 0x6d */
    { "n", "lowercase n" },  /* 0x6e */
    { "o", "lowercase o" },  /* 0x6f */
    { "p", "lowercase p" },  /* 0x70 */
    { "q", "lowercase q" },  /* 0x71 */
    { "r", "lowercase r" },  /* 0x72 */
    { "s", "lowercase s" },  /* 0x73 */
    { "t", "lowercase t" },  /* 0x74 */
    { "u", "lowercase u" },  /* 0x75 */
    { "v", "lowercase v" },  /* 0x76 */
    { "w", "lowercase w" },  /* 0x77 */
    { "x", "lowercase x" },  /* 0x78 */
    { "y", "lowercase y" },  /* 0x79 */
    { "z", "lowercase z" },  /* 0x7a */
    { "opening brace", "left curly bracket" },  /* 0x7b */
    { "vertical bar", NULL },  /* 0x7c */
    { "closing brace", "right curly bracket" },  /* 0x7d */
    { "tilde", NULL },  /* 0x7e */
    { "delete", NULL },  /* 0x7f */
    { NULL, NULL },  /* 0x80 */
    { NULL, NULL },  /* 0x81 */
    { NULL, NULL },  /* 0x82 */
    { NULL, NULL },  /* 0x83 */
    { NULL, NULL },  /* 0x84 */
    { "next line", NULL },  /* 0x85 */
    { NULL, NULL },  /* 0x86 */
    { NULL, NULL },  /* 0x87 */
    { NULL, NULL },  /* 0x88 */
    { NULL, NULL },  /* 0x89 */
    { NULL, NULL },  /* 0x8a */
    { NULL, NULL },  /* 0x8b */
    { NULL, NULL },  /* 0x8c */
    { NULL, NULL },  /* 0x8d */
    { NULL, NULL },  /* 0x8e */
    { NULL, NULL },  /* 0x8f */
    { NULL, NULL },  /* 0x90 */
    { NULL, NULL },  /* 0x91 */
    { NULL, NULL },  /* 0x92 */
    { NULL, NULL },  /* 0x93 */
    { NULL, NULL },  /* 0x94 */
    { NULL, NULL },  /* 0x95 */
    { NULL, NULL },  /* 0x96 */
    { NULL, NULL },  /* 0x97 */
    { NULL, NULL },  /* 0x98 */
    { NULL, NULL },  /* 0x99 */
    { NULL, NULL },  /* 0x9a */
    { NULL, NULL },  /* 0x9b */
    { NULL, NULL },  /* 0x9c */
    { NULL, NULL },  /* 0x9d */
    { NULL, NULL },  /* 0x9e */
    { NULL, NULL },  /* 0x9f */
    { "space", NULL },  /* 0xa0 */
    { "inverted exclamation mark", NULL },  /* 0xa1 */
    { "cent", NULL },  /* 0xa2 */
    { "pound", NULL },  /* 0xa3 */
    { "currency", NULL },  /* 0xa4 */
    { "yen", NULL },  /* 0xa5 */
    { "broken bar", NULL },  /* 0xa6 */
    { "section", NULL },  /* 0xa7 */
    { "diaeresis", NULL },  /* 0xa8 */
    { "copyright", NULL },  /* 0xa9 */
    { "superscript _a", NULL },  /* 0xaa */
    { "angle quote", NULL },  /* 0xab */
    { "negation", NULL },  /* 0xac */
    { "soft hyphen", NULL },  /* 0xad */
    { "registered", NULL },  /* 0xae */
    { "macron", NULL },  /* 0xaf */
    { "degree", NULL },  /* 0xb0 */
    { "plus minus", NULL },  /* 0xb1 */
    { "squared", NULL },  /* 0xb2 */
    { "cubed", NULL },  /* 0xb3 */
    { "acute accent", NULL },  /* 0xb4 */
    { "micro", NULL },  /* 0xb5 */
    { "pilcrow", NULL },  /* 0xb6 */
    { "midlle dot", NULL },  /* 0xb7 */
    { "cedilla", NULL },  /* 0xb8 */
    { "superscript one", NULL },  /* 0xb9 */
    { "superscript o", NULL },  /* 0xba */
    { "angle quote", NULL },  /* 0xbb */
    { "one fourth", NULL },  /* 0xbc */
    { "one half", NULL },  /* 0xbd */
    { "three fourths", NULL },  /* 0xbe */
    { "inverted question mark", NULL },  /* 0xbf */
    { "_a grave", "capital _a grave" },  /* 0xc0 */
    { "_a acute", "capital _a acute" },  /* 0xc1 */
    { "_a circumflex", "capital _a circumflex" },  /* 0xc2 */
    { "_a tilde", "capital _a tilde" },  /* 0xc3 */
    { "_a diaeresis", "capital _a diaeresis" },  /* 0xc4 */
    { "_a ring", "capital _a ring" },  /* 0xc5 */
    { "ash", "capital ash" },  /* 0xc6 */
    { "c cedilla", "capital c cedilla" },  /* 0xc7 */
    { "e grave", "capital e grave" },  /* 0xc8 */
    { "e acute", "capital e acute" },  /* 0xc9 */
    { "e circumflex", "capital e circumflex" },  /* 0xca */
    { "e diaeresis", "capital e diaeresis" },  /* 0xcb */
    { "i grave", "capital i grave" },  /* 0xcc */
    { "i acute", "capital i acute" },  /* 0xcd */
    { "i circumflex", "capital i circumflex" },  /* 0xce */
    { "i diaeresis", "capital i diaeresis" },  /* 0xcf */
    { "eth", "capital eth" },  /* 0xd0 */
    { "n tilde", "capital n tilde" },  /* 0xd1 */
    { "o grave", "capital o grave" },  /* 0xd2 */
    { "o acute", "capital o acute" },  /* 0xd3 */
    { "o circumflex", "capital o circumflex" },  /* 0xd4 */
    { "o tilde", "capital o tilde" },  /* 0xd5 */
    { "o diaeresis", "capital o diaeresis" },  /* 0xd6 */
    { "times", NULL },  /* 0xd7 */
    { "o with stroke", "capital o with stroke" },  /* 0xd8 */
    { "u grave", "capital u grave" },  /* 0xd9 */
    { "u acute", "capital u acute" },  /* 0xda */
    { "u circumflex", "capital u circumflex" },  /* 0xdb */
    { "u diaeresis", "capital u diaeresis" },  /* 0xdc */
    { "y acute", "capital y acute" },  /* 0xdd */
    { "thorn", "capital thorn" },  /* 0xde */
    { "sharp s", NULL },  /* 0xdf */
    { "_a grave", "lowercase _a grave" },  /* 0xe0 */
    { "_a acute", "lowercase _a acute" },  /* 0xe1 */
    { "_a circumflex", "lowercase _a circumflex" },  /* 0xe2 */
    { "_a tilde", "lowercase _a tilde" },  /* 0xe3 */
    { "_a diaeresis", "lowercase _a diaeresis" },  /* 0xe4 */
    { "_a ring", "lowercase _a ring" },  /* 0xe5 */
    { "ash", "lowercase ash" },  /* 0xe6 */
    { "c cedilla", "lowercase c cedilla" },  /* 0xe7 */
    { "e grave", "lowercase e grave" },  /* 0xe8 */
    { "e acute", "lowercase e acute" },  /* 0xe9 */
    { "e circumflex", "lowercase e circumflex" },  /* 0xea */
    { "e diaeresis", "lowercase e diaeresis" },  /* 0xeb */
    { "i grave", "lowercase i grave" },  /* 0xec */
    { "i acute", "lowercase i acute" },  /* 0xed */
    { "i circumflex", "lowercase i circumflex" },  /* 0xee */
    { "i diaeresis", "lowercase i diaeresis" },  /* 0xef */
    { "eth", "lowercase eth" },  /* 0xf0 */
    { "n tilde", "lowercase n tilde" },  /* 0xf1 */
    { "o grave", "lowercase o grave" },  /* 0xf2 */
    { "o acute", "lowercase o acute" },  /* 0xf3 */
    { "o circumflex", "lowercase o circumflex" },  /* 0xf4 */
    { "o tilde", "lowercase o tilde" },  /* 0xf5 */
    { "o diaeresis", "lowercase o diaeresis" },  /* 0xf6 */
    { "divided by", NULL },  /* 0xf7 */
    { "o with stroke", "lowercase o with stroke" },  /* 0xf8 */
    { "u grave", "lowercase u grave" },  /* 0xf9 */
    { "u acute", "lowercase u acute" },  /* 0xfa */
    { "u circumflex", "lowercase u circumflex" },  /* 0xfb */
    { "u diaeresis", "lowercase u diaeresis" },  /* 0xfc */
    { "y acute", "lowercase y acute" },  /* 0xfd */
    { "thorn", "lowercase thorn" },  /* 0xfe */
    { "y diaeresis", "lowercase y diaeresis" },  /* 0xff */
};

/* 0x3d5598: currencies: symbol or code, unit (singular, plural), subunit, a qualifier, flags */
static const en_currency en_currencies[] = {
    { "$", "dollar", "dollars", "cent", "cents", NULL, 0 },
    { "\243", "pound", "pounds", "penny", "pence", NULL, 0 },
    { "\245", "yen", "yen", "sen", "sen", NULL, 0 },
    { "kr", "krona", "kronas", "orah", "orah", NULL, 0 },
    { "Rs", "rupee", "rupees", "paisa", "paise", NULL, 0 },
    { "AUD", "dollar", "dollars", "cent", "cents", "australian", 0 },
    { "BGN", "lev", "leva", "stotinka", "stotinki", "bulgarian", 0 },
    { "BBD", "dollar", "dollars", "cent", "cents", "barbadian", 0 },
    { "BMD", "dollar", "dollars", "cent", "cents", "bermudian", 0 },
    { "BRL", "real", "reais", "centavo", "centavos", "brazilian", 0 },
    { "BSD", "dollar", "dollars", "cent", "cents", "bahamian", 0 },
    { "BZD", "dollar", "dollars", "cent", "cents", "belize", 0 },
    { "CAD", "dollar", "dollars", "cent", "cents", "canadian", 0 },
    { "CHF", "franc", "francs", "centime", "centimes", "swiss", 0 },
    { "CLP", "peso", "pesos", "centavo", "centavos", "chilean", 0 },
    { "CNY", "yuan", "yuans", "fen", "fens", "chinese", 0 },
    { "COP", "peso", "pesos", "centavo", "centavos", "colombian", 0 },
    { "CUC", "peso", "pesos", "centavo", "centavos", "cuban convertible", 0 },
    { "CUP", "peso", "pesos", "centavo", "centavos", "cuban", 0 },
    { "CZK", "koruna", "korun", "heller", "hellers", "czech", 0 },
    { "DEM", "deutschmark", "deutschmarks", "pfennig", "pfennigs", NULL, 0 },
    { "DKK", "krone", "kroner", "orah", "orah", "danish", 0 },
    { "DOP", "peso", "pesos", "centavo", "centavos", "dominican", 0 },
    { "EEK", "kroon", "krooni", "sent", "senti", "estonian", 0 },
    { "EUR", "euro", "euros", "cent", "cents", "euro", 2 },
    { "FJD", "dollar", "dollars", "cent", "cents", "fijian", 0 },
    { "FRF", "franc", "francs", "centime", "centimes", "french", 0 },
    { "GBP", "pound", "pounds", "penny", "pence", NULL, 0 },
    { "GYD", "dollar", "dollars", "cent", "cents", "guyanese", 0 },
    { "HKD", "dollar", "dollars", "cent", "cents", "hong kong", 0 },
    { "HUF", "forint", "forints", "filler", "fillers", "hungarian", 0 },
    { "IDR", "rupiah", "rupiahs", "sen", "sens", "indonesian", 0 },
    { "INR", "rupee", "rupees", "paisa", "paise", "indian", 0 },
    { "ISK", "krona", "kronur", "eyrir", "aurar", "icelandic", 0 },
    { "JMD", "dollar", "dollars", "cent", "cents", "jamaican", 0 },
    { "JPY", "yen", "yen", "sen", "sen", "japanese", 0 },
    { "KRW", "won", "won", "jeon", "jeon", "south korean", 0 },
    { "KYD", "dollar", "dollars", "cent", "cents", "cayman islands", 0 },
    { "LTL", "litas", "litai", "centas", "centai", "lithuanian", 0 },
    { "LVL", "lats", "lati", "santims", "santimi", "latvian", 0 },
    { "MXN", "peso", "pesos", "centavo", "centavos", "mexican", 0 },
    { "NAD", "dollar", "dollars", "cent", "cents", "namibian", 0 },
    { "NIO", "cordoba", "cordobas", "centavo", "centavos", "nicaraguan", 0 },
    { "NOK", "krone", "kroner", "orah", "orah", "norwegian", 0 },
    { "NZD", "dollar", "dollars", "cent", "cents", "new zealand", 0 },
    { "PKR", "rupee", "rupees", "paisa", "paise", "pakistani", 0 },
    { "PLN", "zloty", "zlotys", "grosz", "groszy", "polish", 0 },
    { "RON", "leu", "lei", "ban", "bani", "romanian", 0 },
    { "RUB", "ruble", "rubles", "kopek", "kopeks", "russian", 0 },
    { "SEK", "krona", "kronor", "orah", "orah", "swedish", 0 },
    { "SBD", "dollar", "dollars", "cent", "cents", "solomon islands", 0 },
    { "SGD", "dollar", "dollars", "cent", "cents", "singapore", 0 },
    { "TRY", "lira", "lire", "kurus", "kurus", "turkish", 0 },
    { "TTD", "dollar", "dollars", "cent", "cents", "trinidad and tobago", 0 },
    { "TWD", "dollar", "dollars", "cent", "cents", "taiwan", 0 },
    { "UAH", "hryvnia", "hryvni", "kopiyka", "kopiyok", "ukrainian", 0 },
    { "USD", "dollar", "dollars", "cent", "cents", "u.s.", 0 },
    { "UYU", "peso", "pesos", "centesimo", "centesimos", "uruguayan", 0 },
    { "ZAR", "rand", "rand", "cent", "cents", "south african", 0 },
    { "ZWL", "dollar", "dollars", "cent", "cents", "zimbabwean", 0 },
    { "XCD", "dollar", "dollars", "cent", "cents", "east caribbean", 0 },
    { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

/* 0x3d5c60: nouns after which an amount is singular ("a $5 bill") */
static const char *const en_money_nouns[] = {
    "bab", "banknote", "bill", "bond", "bonus", "bount", "budget", "building", "business", "coin",
    "contract", "deal", "debt", "deficit", "donation", "drawing", "expansion", "facilit", "fine",
    "fund", "grant", "home", "house", "investment", "i.p.o", "jackpot", "lawsuit", "loan", "loss",
    "man", "men", "mansion", "mark", "note", "offer", "l.p.g.a", "p.g.a", "plan", "price", "prize",
    "profit", "project", "question", "range", "renovation", "reward", "salar", "settlement",
    "shortfall", "surplus", "I.P.O", "L.P.G.A", "P.G.A", NULL,
};

/* 0x3d5d38 */
static const char *const digit2num[10] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
};

/* 0x3d5d60 */
static const char *const digit2enty[10] = {
    "zero", "ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety",
};

/* 0x3d5d88 */
static const char *const digit2teen[10] = {
    "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen",
    "eighteen", "nineteen",
};

/* 0x3d5db0: hundred, thousand, million, ... */
static const char *const en_scales[45] = {
    "hundred", "thousand", "million", "billion", "trillion", "quadrillion", "quintillion",
    "sextillion", "septillion", "octillion", "nonillion", "decillion", "undecillion",
    "duodecillion", "tredecillion", "quattuordecillion", "quindecillion", "sexdecillion",
    "septendecillion", "octodecillion", "novemdecillion", "vigintillion", "unvigintillion",
    "duovigintillion", "trevigintillion", "quattuorvigintillion", "quinvigintillion",
    "sexvigintillion", "septvigintillion", "octovigintillion", "novemvigintillion", "trigintillion",
    "untrigintillion", "duotrigintillion", "tretrigintillion", "quattuortrigintillion",
    "quintrigintillion", "sextrigintillion", "septentrigintillion", "octotrigintillion",
    "novemtrigintillion", "quadragintillion", "unquadragintillion", "duoquadragintillion",
    "trequadragintillion",
};

/* 0x3d5e64: their ordinals */
static const char *const en_scales_ord[45] = {
    "hundredth", "thousandth", "millionth", "billionth", "trillionth", "quadrillionth",
    "quintillionth", "sextillionth", "septillionth", "octillionth", "nonillionth", "decillionth",
    "undecillionth", "duodecillionth", "tredecillionth", "quattuordecillionth", "quindecillionth",
    "sexdecillionth", "septendecillionth", "octodecillionth", "novemdecillionth", "vigintillionth",
    "unvigintillionth", "duovigintillionth", "trevigintillionth", "quattuorvigintillionth",
    "quinvigintillionth", "sexvigintillionth", "septvigintillionth", "octovigintillionth",
    "novemvigintillionth", "trigintillionth", "untrigintillionth", "duotrigintillionth",
    "tretrigintillionth", "quattuortrigintillionth", "quintrigintillionth", "sextrigintillionth",
    "septentrigintillionth", "octotrigintillionth", "novemtrigintillionth", "quadragintillionth",
    "unquadragintillionth", "duoquadragintillionth", "trequadragintillionth",
};

/* 0x3d5f18 */
static const char *const en_era_ad[] = {
    "AD", "ad", NULL,
};

/* 0x3d5f24 */
static const char *const en_era_ad_dotted[] = {
    "A.D", "a.d", NULL,
};

/* 0x3d5f30 */
static const char *const en_era_bc[] = {
    "BC", "CE", "BCE", "bc", "ce", "bce", NULL,
};

/* 0x3d5f4c */
static const char *const en_era_bc_dotted[] = {
    "B.C", "C.E", "B.C.E", "b.c", "c.e", "b.c.e", NULL,
};

/* 0x3d5f68 */
static const char *const en_months[12] = {
    "january", "february", "march", "april", "may", "june", "july", "august", "september",
    "october", "november", "december",
};

static inline int is_british(const cst_item *t)
{
    return strcmp(ffeature_string(item_traverse(t, 0), "language_code"), "en-AU") == 0
           || cst_startswith(ffeature_string(item_traverse(t, 0), "language_code"), "en-GB")
           || strcmp(ffeature_string(item_traverse(t, 0), "language_code"), "en-IN") == 0;
}

/* 0x4d0b8 arm: the names of the characters of s (their other names when alt) */
cst_val *en_exp_chars(const cst_item *t, const char *s, int alt)
{
    cst_val *r = NULL;
    const char *name;
    unsigned int c;

    for (; (c = (unsigned char)*s) != 0; s++) {
        if (is_british(t) && (c == 0xbc || c == 0xbe))
            name = c == 0xbc ? "one quarter" : "three quarters";
        else if (alt && en_char_names[c][1])
            name = en_char_names[c][1];
        else if ((name = en_char_names[c][0]) == NULL)
            continue;
        r = val_append(r, cst_split_tokens_string(name));
    }
    return r;
}

/* 0x4d240 arm */
cst_val *en_exp_letters(const cst_item *t, const char *s)
{
    return en_exp_chars(t, s, 0);
}

/* 0x4e434 arm: parses an amount of money ("$1,000", "-€5.50", "£3M") into m */
int en_parse_money(const char *s, en_money *m)
{
    size_t cl, start, n;
    int sign;
    char c;

    if (!cst_regex_match(en_rx_money, s))
        return 0;
    sign = *s;
    if (strchr("-+\261", sign))
        s++;
    else
        sign = 0;
    m->sign = sign;
    if ((cl = strcspn(s, "0123456789,.+-\261")) == 0)
        return 0;
    c = s[cl];
    start = cl;
    if (strchr("-+\261", c)) {
        if (sign)
            return 0;
        m->sign = c;
        start = cl + 1;
    }
    if ((n = strlen(s + start)) == 0)
        return 0;
    if (strchr("MBTKk", s[start + n - 1])) {
        n--;
        m->scale = s + start + n;
        if (n == 0)
            return 0;
    } else
        m->scale = NULL;
    m->currency = cst_substr(s, 0, cl);
    m->amount = cst_substr(s, start, n);
    return 1;
}

/* the unit words, with the currency's qualifier where it goes */
static inline cst_val *currency_words(const en_currency *c, const char *unit)
{
    cst_val *r = cst_split_tokens_string(unit);

    if (c->qualifier && !(c->flags & 2)) {
        if (c->flags & 1)
            r = val_append(r, cst_split_tokens_string(c->qualifier));
        else
            r = val_append(cst_split_tokens_string(c->qualifier), r);
    }
    return r;
}

cst_val *en_exp_number(const cst_item *t, const char *s);
cst_val *en_exp_real(const cst_item *t, const char *s);

/* 0x4def8 arm: amount (NULL: just the unit) of the currency cur: "five dollars and ten cents" */
cst_val *en_exp_currency(const cst_item *t, const char *amount, const char *cur)
{
    const en_currency *c;
    const char *dot, *cents, *unit;
    cst_val *w, *cw, *r;
    char *whole, buf[3];
    int singular;

    for (c = en_currencies; c->symbol; c++)
        if (strcmp(c->symbol, cur) == 0)
            break;
    singular = cst_startswith_any(ffeature_string(item_traverse(t, 0x100, 0), "name"),
                                  en_money_nouns);
    if (c->symbol == NULL)
        return NULL;
    if (amount == NULL)
        return currency_words(c, singular ? c->unit : c->units);
    if ((dot = (const char *)cst_strrchr((const cst_string *)amount, '.')) != NULL && strlen(cents = dot + 1) < 3) {
        whole = cst_strdup(amount);
        whole[strlen(amount) - strlen(dot)] = '\0';
        if (strcmp("0", whole) == 0)
            w = NULL;
        else {
            w = en_exp_real(t, whole);
            unit = singular || strcmp("1", whole) == 0 || strcmp("01", whole) == 0 ? c->unit
                                                                                     : c->units;
            w = val_append(w, currency_words(c, unit));
        }
        cst_free(whole);
        if (strlen(cents) == 1)
            snprintf(buf, 3, "%c0", dot[1]);
        else
            snprintf(buf, 3, "%s", cents);
        r = w;
        if (strcmp("00", buf) != 0) {
            cw = en_exp_number(t, buf);
            unit = singular || strcmp("1", buf) == 0 || strcmp("01", buf) == 0 ? c->subunit
                                                                                 : c->subunits;
            if (c->qualifier && w == NULL) {
                r = cst_split_tokens_string(unit);
                if (c->flags & 1)
                    r = val_append(r, cst_split_tokens_string(c->qualifier));
                else
                    r = val_append(cst_split_tokens_string(c->qualifier), r);
                r = val_append(cw, r);
            } else {
                r = val_append(cw, cst_split_tokens_string(unit));
                if (w)
                    r = val_append(w, cons_val(string_val("and"), r));
            }
        }
        if (r)
            return r;
        return cons_val(string_val("zero"), currency_words(c, singular ? c->unit : c->units));
    }
    w = en_exp_real(t, amount);
    unit = singular || strcmp("1", amount) == 0 || strcmp("01", amount) == 0 ? c->unit : c->units;
    return val_append(w, currency_words(c, unit));
}

/* 0x4e8ac arm: s without commas */
void en_strip_commas(char *s)
{
    char *p;
    int j = 0;

    for (p = s; *p; p++)
        if (*p != ',')
            s[j++] = *p;
    s[j] = '\0';
}

cst_val *en_exp_sign(const cst_item *t, int c);

/* 0x4e570 arm: an amount of money: the sign, the number (with "thousand", ... for a scale)
 * and the currency */
cst_val *en_exp_money(const cst_item *t, en_money *m)
{
    cst_val *r = en_exp_sign(t, m->sign);
    const char *scale = m->scale;
    char *a;
    size_t n;

    if (scale && (strcmp(scale, "k") == 0 || strcmp(scale, "K") == 0)) {
        en_strip_commas(m->amount);
        n = strlen(m->amount);
        if (!strchr(m->amount, '.') && n < 10) {
            a = cst_safe_alloc(n + 4, 1);
            memcpy(a, m->amount, n);
            memcpy(a + n, "000", 3);
            a[n + 3] = '\0';
            cst_free(m->amount);
            m->amount = a;
            m->scale = NULL;
            return val_append(r, en_exp_currency(t, m->amount, m->currency));
        }
    } else if (scale == NULL)
        return val_append(r, en_exp_currency(t, m->amount, m->currency));
    r = val_append(r, en_exp_real(t, m->amount));
    scale = m->scale;
    if (strcmp(scale, "k") == 0 || strcmp(scale, "K") == 0)
        r = val_append(r, cons_val(string_val("thousand"), NULL));
    else if (strcmp(scale, "M") == 0)
        r = val_append(r, cons_val(string_val("million"), NULL));
    else if (strcmp(scale, "B") == 0)
        r = val_append(r, cons_val(string_val("billion"), NULL));
    else if (strcmp(scale, "T") == 0)
        r = val_append(r, cons_val(string_val("trillion"), NULL));
    return val_append(r, en_exp_currency(t, NULL, m->currency));
}

/* 0x4e7c8 arm: two digits */
cst_val *en_exp_2digits(const cst_item *t, const char *s)
{
    if (s[0] == '0') {
        if (s[1] == '0')
            return NULL;
        return cons_val(string_val(digit2num[s[1] - '0']), NULL);
    }
    if (s[1] == '0')
        return cons_val(string_val(digit2enty[s[0] - '0']), NULL);
    if (s[0] == '1')
        return cons_val(string_val(digit2teen[s[1] - '0']), NULL);
    return cons_val(string_val(digit2enty[s[0] - '0']), en_exp_letters(t, s + 1));
}

/* 0x4e8ec arm: s digit by digit */
cst_val *en_exp_digits(const cst_item *t, const char *s)
{
    cst_val *d = NULL;

    for (; *s; s++)
        d = cons_val(string_val((unsigned char)(*s - '0') < 10 ? digit2num[*s - '0'] : "umpty"), d);
    return val_reverse(d);
}

/* 0x4e964 arm: a digit string as a number; British English puts "and" before the tens and
 * units of a hundred (unless at the start, after_scale 0, and "0..") */
cst_val *en_exp_number_r(const cst_item *t, const char *s, int after_scale)
{
    size_t n = strlen(s), k;
    char part[4];
    cst_val *p;

    if (n == 0)
        return NULL;
    if (n == 1)
        return cons_val(string_val(digit2num[s[0] - '0']), NULL);
    if (n == 2)
        return en_exp_2digits(t, s);
    if (n == 3) {
        p = en_exp_2digits(t, s + 1);
        if (p) {
            if (after_scale == 0 && s[0] == '0')
                return p;
            if (is_british(t))
                p = cons_val(string_val("and"), p);
        }
        if (s[0] != '0')
            p = cons_val(string_val(digit2num[s[0] - '0']), cons_val(string_val("hundred"), p));
        return p;
    }
    if (n < 0x88) {
        k = (n - 1) % 3 + 1;
        memcpy(part, s, k);
        part[k] = '\0';
        if ((p = en_exp_number_r(t, part, 0)) == NULL)
            return en_exp_number_r(t, s + k, after_scale);
        return val_append(p, cons_val(string_val(en_scales[(n - 1) / 3]),
                                      en_exp_number_r(t, s + k, 1)));
    }
    if (!is_british(t))
        return en_exp_digits(t, s);
    return en_exp_letters(t, s);
}

/* 0x4ec78 arm: a number (commas allowed, leading zeros ignored), NULL if s is not one */
cst_val *en_exp_number(const cst_item *t, const char *s)
{
    char *d = cst_strdup(s), *p;
    cst_val *r = NULL;
    size_t n, i;

    en_strip_commas(d);
    if (cst_regex_match(cst_rx_digits, d)) {
        n = strlen(d);
        for (p = d, i = 0; i + 1 < n && *p == '0'; p++, i++)
            ;
        r = en_exp_number_r(t, p, 0);
    }
    cst_free(d);
    return r;
}

/* 0x4ed14 arm: the ordinal of a number */
cst_val *en_exp_ordinal(const cst_item *t, const char *s)
{
    cst_val *card = val_reverse(en_exp_number(t, s)), *o;
    const cst_val *v;
    const char *l = val_string(val_car(card)), *ord = NULL;
    int i;

    for (i = 0; i < 10; i++)
        if (strcmp(l, digit2num[i]) == 0)
            ord = ord2num[i];
    if (!ord)
        for (i = 0; i < 10; i++)
            if (strcmp(l, digit2teen[i]) == 0)
                ord = ord2teen[i];
    if (!ord)
        for (i = 0; i < 10; i++)
            if (strcmp(l, digit2enty[i]) == 0)
                ord = ord2enty[i];
    for (i = 0; i < 45; i++)
        if (strcmp(l, en_scales[i]) == 0) {
            ord = en_scales_ord[i];
            break;
        }
    if (!ord)
        return val_reverse(card);
    o = cons_val(string_val(ord), NULL);
    for (v = val_cdr(card); v; v = val_cdr(v))
        o = cons_val(val_car(v), o);
    delete_val(card);
    return o;
}

/* 0x4f24c arm: a number said in pairs, as years and ids are: "nineteen oh five" */
cst_val *en_exp_id(const cst_item *t, const char *s)
{
    char aaa[3];
    size_t n;

    if (!*s)
        return NULL;
    n = strlen(s);
    aaa[0] = s[0];
    if (n == 4) {
        if (s[2] == '0' && s[3] == '0') {
            if (s[1] != '0') {
                aaa[1] = s[1];
                aaa[2] = '\0';
                return val_append(en_exp_number(t, aaa), cons_val(string_val("hundred"), NULL));
            }
            return en_exp_number(t, s);
        }
        aaa[1] = s[1];
        if (s[1] != '0') {
            if (s[0] == '1' && s[2] == '0') {
                aaa[2] = '\0';
                return val_append(en_exp_number(t, aaa),
                                  cons_val(string_val("oh"), en_exp_digits(t, s + 3)));
            }
        } else {
            if (s[2] == '0')
                return en_exp_number(t, s);
            aaa[1] = '0';
        }
    } else {
        if (n == 3 && s[0] != '0' && s[1] == '0' && s[2] == '0') {
            aaa[1] = '\0';
            return val_append(en_exp_number(t, aaa), cons_val(string_val("hundred"), NULL));
        }
        if (n == 2 && s[0] == '0')
            return cons_val(string_val("oh"), en_exp_letters(t, s + 1));
        if (n <= 2)
            return en_exp_number(t, s);
        if (n & 1)
            return cons_val(string_val(digit2num[s[0] - '0']), en_exp_id(t, s + 1));
        aaa[1] = s[1];
    }
    aaa[2] = '\0';
    return val_append(en_exp_number(t, aaa), en_exp_id(t, s + 2));
}

/* 0x4f4c8 arm: a sign */
cst_val *en_exp_sign(const cst_item *t, int c)
{
    char s[2];

    if (c == 0)
        return NULL;
    if (c == '-')
        return cons_val(string_val("minus"), NULL);
    if (c == '+')
        return cons_val(string_val("plus"), NULL);
    s[0] = c;
    s[1] = '\0';
    return en_exp_letters(t, s);
}

/* 0x4f54c arm: a real number: sign, "e" exponent, "point" and digits */
cst_val *en_exp_real(const cst_item *t, const char *s)
{
    const char *p;
    char *d;
    cst_val *r;

    if (s && strchr("-+\302\261", *s))
        return val_append(en_exp_sign(t, *s), en_exp_real(t, s + 1));
    if ((p = strchr(s, 'e')) != NULL || (p = strchr(s, 'E')) != NULL) {
        d = cst_strdup(s);
        d[strlen(s) - strlen(p)] = '\0';
        r = val_append(en_exp_real(t, d), cons_val(string_val("e"), en_exp_real(t, p + 1)));
    } else if ((p = strchr(s, '.')) != NULL) {
        d = cst_strdup(s);
        d[strlen(s) - strlen(p)] = '\0';
        r = val_append(en_exp_number(t, d),
                       cons_val(string_val("point"), en_exp_digits(t, p + 1)));
    } else
        return en_exp_number(t, s);
    cst_free(d);
    return r;
}

/* 0x4f98c arm: a time h(:mm(:ss)) ("o'clock" or "hundred hours" on the hour unless
 * no_oclock) */
cst_val *en_exp_time(const cst_item *t, const char *s, int no_oclock)
{
    const char *c1, *c2, *m;
    char *hour = NULL, *mins = NULL;
    cst_val *rest = NULL, *secs;

    if ((c1 = strchr(s, ':')) != NULL) {
        m = c1 + 1;
        c2 = strchr(m, ':');
        if ((hour = cst_substr(s, 0, c1 - s)) != NULL)
            s = hour;
        if (c2 == NULL) {
            secs = NULL;
            if (strcmp("00", m) != 0) {
                rest = val_append(en_exp_id(t, m), secs);
                goto done;
            }
        } else {
            if (strcmp("01", c2 + 1) == 0)
                secs = cons_val(string_val("and"),
                                cons_val(string_val("one"), cons_val(string_val("second"), NULL)));
            else if (strcmp("00", c2 + 1) == 0)
                secs = NULL;
            else
                secs = cons_val(string_val("and"),
                                val_append(en_exp_number(t, c2 + 1),
                                           cons_val(string_val("seconds"), NULL)));
            mins = cst_substr(c1, 1, c2 - c1 - 1);
            if (strcmp("00", mins) != 0) {
                rest = val_append(en_exp_id(t, mins), secs);
                goto done;
            }
            if (secs) {
                rest = cons_val(string_val("hours"), secs);
                goto done;
            }
        }
        if (!no_oclock) {
            if (atoi(s) < 13)
                rest = cons_val(string_val("o'clock"), NULL);
            else
                rest = cons_val(string_val("hundred"), cons_val(string_val("hours"), NULL));
        }
    done:
        cst_free(mins);
    }
    rest = val_append(en_exp_number(t, s), rest);
    cst_free(hour);
    return rest;
}

/* 0x50268 arm: a UTC offset ("GMT+5", "UTC-03:30") */
cst_val *en_exp_utc_offset(const cst_item *t, const char *s)
{
    char *zone = cst_substr(s, 0, 3), *d, *h;
    const char *rest = s + 4, *mm;
    size_t n = strlen(rest);
    cst_val *r = en_exp_letters(t, zone);

    d = cst_strdup(rest);
    r = val_append(r, en_exp_sign(t, s[3]));
    if (n < 3)
        mm = NULL;
    else {
        n -= 2;
        mm = rest + n;
        d[n] = '\0';
    }
    if (d[n - 1] == ':')
        d[n - 1] = '\0';
    h = *d == '0' ? d + 1 : d;
    r = val_append(r, en_exp_number(t, h));
    if (mm && strcmp(mm, "00") != 0)
        r = val_append(r, en_exp_number(t, mm));
    cst_free(zone);
    cst_free(d);
    return r;
}

/* 0x5037c arm: whether an era ("AD") precedes the token */
int en_era_before(const cst_item *t)
{
    const char *name;

    if (!token_no_punc_prev(t))
        return 0;
    name = item_feat_string(item_prev(t), "name");
    return cst_member_string(name, en_era_ad) || cst_member_string(name, en_era_ad_dotted);
}

/* 0x503e8 arm: whether an era ("AD", "BC", "B.C.") follows the token */
int en_era_after(const cst_item *t)
{
    const char *name;

    if (!token_no_punc_next(t))
        return 0;
    name = item_feat_string(item_next(t), "name");
    if (cst_member_string(name, en_era_ad) || cst_member_string(name, en_era_bc))
        return 1;
    if (!cst_member_string(name, en_era_ad_dotted) && !cst_member_string(name, en_era_bc_dotted))
        return 0;
    return en_is_dotted_abbrev(item_next(t), name);
}

/* 0x50490 arm: a date in the given field order ("mdy", "dmy", "ymd", "y"): "the fifth of
 * May twenty twelve" in British English, "May fifth twenty twelve" otherwise; NULL if it is not
 * a date */
cst_val *en_exp_date(const cst_item *t, const char *s, const char *order)
{
    cst_date d;
    const char *month = NULL, *year;
    const cst_item *p;
    char buf[8], *low;
    cst_val *r = NULL;
    size_t n;
    int i;

    date_parse(&d, s, order, cst_rx_alpha, NULL);
    if (d.month) {
        if (!cst_regex_match(cst_rx_digits, d.month)) {
            if (strlen(d.month) <= 2)
                goto out;
            low = en_lowercase(d.month);
            n = strlen(low);
            for (i = 0; i < 12; i++)
                if (strncmp(en_months[i], low, n) == 0)
                    month = en_months[i];
            cst_free(low);
        } else if ((unsigned int)(atoi(d.month) - 1) < 12)
            month = en_months[atoi(d.month) - 1];
        if (month == NULL)
            goto out;
    }
    if (d.day && (strlen(d.day) > 2 || !cst_regex_match(cst_rx_digits, d.day)
                  || (unsigned int)(atoi(d.day) - 1) > 30))
        goto out;
    year = d.year;
    if (d.year) {
        if (!cst_regex_match(cst_rx_digits, d.year))
            goto out;
        if (strlen(d.year) == 2 && !en_era_before(t) && !en_era_after(t)) {
            if (token_no_punc_prev(t)) {
                p = item_prev(t);
                if (strcmp("to", item_feat_string(p, "name")) == 0 && token_no_punc_prev(p)
                    && cst_regex_match(cst_rx_digits, item_feat_string(item_prev(p), "name"))
                    && en_era_before(item_prev(p)))
                    goto words;
            }
            if (token_no_punc_next(t)) {
                p = item_next(t);
                if (strcmp("to", item_feat_string(p, "name")) == 0 && token_no_punc_next(p)
                    && cst_regex_match(cst_rx_digits, item_feat_string(item_next(p), "name"))
                    && en_era_after(item_next(p)))
                    goto words;
            }
            snprintf(buf, 5, strchr("012", *d.year) ? "20%s" : "19%s", d.year);
            year = buf;
        }
    }
words:
    if (is_british(t)) {
        if (d.day) {
            r = cons_val(string_val("the"), en_exp_ordinal(t, d.day));
            if (month)
                r = val_append(val_append(r, cons_val(string_val("of"), NULL)),
                               cons_val(string_val(month), NULL));
        } else if (month)
            r = cons_val(string_val(month), NULL);
    } else {
        if (month)
            r = cons_val(string_val(month), NULL);
        if (d.day)
            r = val_append(r, en_exp_ordinal(t, d.day));
    }
    if (year)
        r = val_append(r, en_exp_id(t, year));
out:
    date_free(&d);
    return r;
}

/* 0x50b50 arm: whether s is an era */
int en_is_era(const char *s)
{
    return cst_member_string(s, en_era_ad) || cst_member_string(s, en_era_bc);
}

/* 0x5fcf0 arm (elsewhere in the binary) */
void en_money_free(en_money *m)
{
    cst_free(m->currency);
    cst_free(m->amount);
}
