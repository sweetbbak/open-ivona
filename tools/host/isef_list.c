/* isef_list LANG.DAT [SECTION]: lists a lang.dat's sections and prints SECTION (tools/host/build.sh) */
#include <stdio.h>
#include <string.h>
typedef struct isef_file isef_file;
isef_file *isef_open(const char *path);
const char *isef_next(const isef_file *f, const char *p, size_t n, const char **data, size_t *size);
int isef_find(const isef_file *f, const char *name, const char **data, size_t *size);
int main(int argc, char **argv)
{
    isef_file *f = isef_open(argv[1]);
    const char *name, *d = NULL;
    size_t n = 0;
    if (!f) { printf("open failed\n"); return 1; }
    for (name = isef_next(f, NULL, 0, &d, &n); name; name = isef_next(f, d, n, &d, &n))
        printf("%-30s %8zu%s\n", name, n, (strstr(name, "list") && n < 400) ? "" : "");
    if (argc > 2 && isef_find(f, argv[2], &d, &n)) { fwrite(d, 1, n, stdout); printf("\n"); }
    return 0;
}
