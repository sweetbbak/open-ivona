/*
 * Basic system utilities: directory check and pseudo-random numbers.
 */
#include <sys/stat.h>

/* 0x33664 arm */
int ivona_is_dir(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
        return (st.st_mode >> 14) & 1;
    return 0;
}

/* 0x339dc arm: POSIX-style linear congruential generator */
int ivona_rand_r(unsigned int *seed)
{
    *seed = *seed * 1103515245 + 12345;
    return (*seed >> 16) & 0x7fff;
}

/* 0x33a04 arm */
int ivona_rand(unsigned int seed)
{
    return ivona_rand_r(&seed);
}
