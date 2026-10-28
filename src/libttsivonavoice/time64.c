/*
 * 64-bit time conversions (0x4abd8..0x4c018): Michael G Schwern's y2038 time64.c (MIT
 * license), a revision between bionic's (2008) and the current one: int tables, the newer
 * cycle_offset/safe_year and 400-year shortcut, the older mktime64. struct TM is struct tm
 * (USE_TM64 undefined); the system has no localtime_r/gmtime_r (fake_*_r are used). Asserts
 * are compiled out (NDEBUG).
 */
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef int64_t Time64_T;
typedef int64_t Year;
#define TM tm

static const int days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static const int julian_days_by_month[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};

static const int length_of_year[2] = { 365, 366 };

/* Some numbers relating to the gregorian cycle */
static const Year years_in_gregorian_cycle = 400;
#define days_in_gregorian_cycle ((365 * 400) + 100 - 4 + 1)
static const Time64_T seconds_in_gregorian_cycle = days_in_gregorian_cycle * 60LL * 60LL * 24LL;

/* Year range we can trust the time funcitons with */
#define MAX_SAFE_YEAR 2037
#define MIN_SAFE_YEAR 1971

/* 28 year Julian calendar cycle */
#define SOLAR_CYCLE_LENGTH 28

/* Year cycle from MAX_SAFE_YEAR down. */
static const int safe_years_high[SOLAR_CYCLE_LENGTH] = {
    2016, 2017, 2018, 2019,
    2020, 2021, 2022, 2023,
    2024, 2025, 2026, 2027,
    2028, 2029, 2030, 2031,
    2032, 2033, 2034, 2035,
    2036, 2037, 2010, 2011,
    2012, 2013, 2014, 2015
};

/* Year cycle from MIN_SAFE_YEAR up */
static const int safe_years_low[SOLAR_CYCLE_LENGTH] = {
    1996, 1997, 1998, 1971,
    1972, 1973, 1974, 1975,
    1976, 1977, 1978, 1979,
    1980, 1981, 1982, 1983,
    1984, 1985, 1986, 1987,
    1988, 1989, 1990, 1991,
    1992, 1993, 1994, 1995,
};

#define IS_LEAP_ABS(n) (!((n) % 400) || (!((n) % 4) && ((n) % 100)))

static int is_exception_century(Year year)
{
    int is_exception = ((year % 100 == 0) && !(year % 400 == 0));

    return is_exception;
}

static Year cycle_offset(Year year)
{
    const Year start_year = 2000;
    Year year_diff = year - start_year;
    Year exceptions;

    if (year > start_year)
        year_diff--;

    exceptions = year_diff / 100;
    exceptions -= year_diff / 400;

    return exceptions * 16;
}

static int safe_year(const Year year)
{
    int safe_year;
    Year year_cycle;

    if (year >= MIN_SAFE_YEAR && year <= MAX_SAFE_YEAR)
        return (int)year;

    year_cycle = year + cycle_offset(year);

    /* safe_years_low is off from safe_years_high by 8 years */
    if (year < MIN_SAFE_YEAR)
        year_cycle -= 8;

    /* Change non-leap xx00 years to an equivalent */
    if (is_exception_century(year))
        year_cycle += 11;

    /* Also xx01 years, since the previous year will be wrong */
    if (is_exception_century(year - 1))
        year_cycle += 17;

    year_cycle %= SOLAR_CYCLE_LENGTH;
    if (year_cycle < 0)
        year_cycle = SOLAR_CYCLE_LENGTH + year_cycle;

    if (year < MIN_SAFE_YEAR)
        safe_year = safe_years_low[year_cycle];
    else if (year > MAX_SAFE_YEAR)
        safe_year = safe_years_high[year_cycle];

    return safe_year;
}

/* 0x4af00 arm */
void copy_tm_to_TM64(const struct tm *src, struct TM *dest)
{
    if (src == NULL)
        memset(dest, 0, sizeof(*dest));
    else
        memcpy(dest, src, sizeof(*dest));
}

/* 0x4af1c arm */
void copy_TM64_to_tm(const struct TM *src, struct tm *dest)
{
    if (src == NULL)
        memset(dest, 0, sizeof(*dest));
    else
        memcpy(dest, src, sizeof(*dest));
}

/* 0x4af38 arm: localtime_r() simulated */
struct tm *fake_localtime_r(const time_t *time, struct tm *result)
{
    const struct tm *static_result = localtime(time);

    if (static_result == NULL) {
        memset(result, 0, sizeof(*result));
        return NULL;
    } else {
        memcpy(result, static_result, sizeof(*result));
        return result;
    }
}

/* 0x4af7c arm: gmtime_r() simulated */
struct tm *fake_gmtime_r(const time_t *time, struct tm *result)
{
    const struct tm *static_result = gmtime(time);

    if (static_result == NULL) {
        memset(result, 0, sizeof(*result));
        return NULL;
    } else {
        memcpy(result, static_result, sizeof(*result));
        return result;
    }
}

static Time64_T seconds_between_years(Year left_year, Year right_year)
{
    int increment = (left_year > right_year) ? 1 : -1;
    Time64_T seconds = 0;
    Year cycles;

    if (left_year > 2400) {
        cycles = (left_year - 2400) / 400;
        left_year -= cycles * 400;
        seconds += cycles * seconds_in_gregorian_cycle;
    } else if (left_year < 1600) {
        cycles = (left_year - 1600) / 400;
        left_year += cycles * 400;
        seconds += cycles * seconds_in_gregorian_cycle;
    }

    while (left_year != right_year) {
        seconds += length_of_year[IS_LEAP_ABS(right_year)] * 60 * 60 * 24;
        right_year += increment;
    }

    return seconds * increment;
}

/* 0x4afc0 arm */
Time64_T mktime64(const struct TM *input_date)
{
    struct tm safe_date;
    struct TM date;
    Time64_T time;
    Year year = input_date->tm_year + 1900;

    if (MIN_SAFE_YEAR <= year && year <= MAX_SAFE_YEAR) {
        copy_TM64_to_tm(input_date, &safe_date);
        return (Time64_T)mktime(&safe_date);
    }

    /* Have to make the year safe in date else it won't fit in safe_date */
    date = *input_date;
    date.tm_year = safe_year(year) - 1900;
    copy_TM64_to_tm(&date, &safe_date);

    time = (Time64_T)mktime(&safe_date);

    time += seconds_between_years(year, (Year)(safe_date.tm_year + 1900));

    return time;
}

/* 0x4b428 arm: because I think mktime() is a crappy name */
Time64_T timelocal64(const struct TM *date)
{
    return mktime64(date);
}
