/*
 * Flite cst_mmap_posix.c as modified by IVONA: no perror() diagnostics, mmap of a file range,
 * and cst_read_whole_file() that also reads pipes/character devices (and "-" for stdin).
 */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "cst_file.h"
#include "cst_alloc.h"

/* 0x521b4: map `size` bytes (0: the whole file) starting at `offset`, rounded up to pages. */
cst_filemap *cst_mmap_file_range(const char *path, off_t offset, size_t size)
{
    cst_filemap *fmap;
    size_t pgsize = getpagesize();
    struct stat buf;
    int fd;

    if ((fd = open(path, O_RDONLY)) < 0)
        return NULL;
    if (size == 0) {
        if (fstat(fd, &buf) < 0) {
            close(fd);
            return NULL;
        }
        size = buf.st_size;
    }
    fmap = cst_alloc(cst_filemap, 1);
    fmap->fd = fd;
    fmap->mapsize = (size + pgsize - 1) / pgsize * pgsize;
    if ((fmap->mem = mmap(0, fmap->mapsize, PROT_READ, MAP_SHARED, fd, offset)) == MAP_FAILED) {
        cst_free(fmap);
        return NULL;
    }
    return fmap;
}

/* 0x5227c */
cst_filemap *cst_mmap_file(const char *path)
{
    return cst_mmap_file_range(path, 0, 0);
}

/* 0x52288 */
int cst_munmap_file(cst_filemap *fmap)
{
    if (munmap(fmap->mem, fmap->mapsize) < 0)
        return -1;
    if (close(fmap->fd) < 0)
        return -1;
    cst_free(fmap);
    return 0;
}

/* 0x522cc: read a whole file into memory; "-" is stdin; pipes/character devices are streamed. */
cst_filemap *cst_read_whole_file(const char *path)
{
    cst_filemap *fmap;
    struct stat buf;
    ssize_t n;
    int fd;

    if (strcmp("-", path) != 0) {
        if ((fd = open(path, O_RDONLY)) < 0)
            return NULL;
    } else {
        fd = 0;
    }
    if (fstat(fd, &buf) < 0) {
        close(fd);
        return NULL;
    }
    if (S_ISFIFO(buf.st_mode) || S_ISCHR(buf.st_mode))
        goto stream;

    fmap = cst_alloc(cst_filemap, 1);
    fmap->mapsize = buf.st_size;
    fmap->fd = fd;
    fmap->mem = cst_alloc(char, fmap->mapsize);
    n = read(fmap->fd, fmap->mem, fmap->mapsize);
    if (n < 0 || (size_t)n < fmap->mapsize) {
        close(fmap->fd);
        cst_free(fmap->mem);
        cst_free(fmap);
        return NULL;
    }
    return fmap;

stream:
    {
        size_t len = 0;
        size_t blksize = buf.st_blksize;

        fmap = cst_alloc(cst_filemap, 1);
        fmap->fd = fd;
        fmap->mapsize = blksize ? blksize : 1024;
        fmap->mem = cst_alloc(char, fmap->mapsize);
        for (;;) {
            n = read(fd, (char *)fmap->mem + len, fmap->mapsize - len);
            if (n == 0)
                break;
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                close(fmap->fd);
                cst_free(fmap->mem);
                cst_free(fmap);
                return NULL;
            }
            len += n;
            if (fmap->mapsize - len < blksize) {
                fmap->mapsize *= 2;
                fmap->mem = cst_realloc(fmap->mem, char, fmap->mapsize);
            }
        }
        fmap->mapsize = len;
        fmap->mem = cst_realloc(fmap->mem, char, len);
        return fmap;
    }
}

/* 0x524c0 */
int cst_free_whole_file(cst_filemap *fmap)
{
    if (close(fmap->fd) < 0)
        return -1;
    cst_free(fmap->mem);
    cst_free(fmap);
    return 0;
}

/* 0x524f8 */
cst_filemap *cst_read_part_file(const char *path)
{
    cst_filemap *fmap;
    struct stat buf;
    cst_file fh;

    if ((fh = cst_fopen(path, CST_OPEN_READ)) == NULL)
        return NULL;
    if (fstat(fileno(fh), &buf) < 0) {
        cst_fclose(fh);
        return NULL;
    }
    fmap = cst_alloc(cst_filemap, 1);
    fmap->fh = fh;
    fmap->mapsize = buf.st_size;
    return fmap;
}

/* 0x5255c */
int cst_free_part_file(cst_filemap *fmap)
{
    if (cst_fclose(fmap->fh) < 0)
        return -1;
    cst_free(fmap);
    return 0;
}
