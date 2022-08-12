#if defined(_MSC_VER)
#if !defined _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif /* _CRT_SECURE_NO_WARNINGS */
#endif /* _MSC_VER */
#if !defined _WIN32
#if !defined _GNU_SOURCE
#define _GNU_SOURCE 1
#endif /* _GNU_SOURCE */
#endif /* _WIN32 */
#include "pipe.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#if defined(_WIN32)
#include <Windows.h>
#else /* !_WIN32 */
#include <unistd.h>
#endif /* _WIN32 */

static char *path_join(char *path, const char *name)
{
    char *cur = path + strlen(path);
    while (cur != path)
    {
        if (*cur == '/' || *cur == '\\')
        {
            ++cur;
            break;
        }
        --cur;
    }
    strcpy(cur, name);
    return path;
}

int main(int argc, char *argv[])
{
    size_t n = strtoul(argv[argc - 1], 0, 0);
    pipe_s *ctx = (pipe_s *)malloc(sizeof(pipe_s) * n);

    char *line = 0;
    char buf[BUFSIZ];
#if defined(_WIN32)
    GetModuleFileName(NULL, buf, (DWORD)(BUFSIZ - 1));
#else /* !_WIN32 */
    readlink("/proc/self/exe", buf, BUFSIZ - 1);
#endif /* _WIN32 */

#if defined(_WIN32)
    const char *path = path_join(buf, "sum.exe");
#else /* !_WIN32 */
    const char *path = path_join(buf, "sum");
#endif /* _WIN32 */

    char *args[] = {"sum", "ok", NULL};
    char *envp[] = {NULL};

    line = pipe_line_envp((char *[]){"/usr/local/bin", "/usr/bin", "/bin", 0});
    if (line)
    {
#if defined(_WIN32)
        printf("path: ");
        for (const char *str = line; *str;)
        {
            str += printf("%s ", str);
        }
        putchar('\n');
#else /* !_WIN32 */
        printf("path: %s\n", line);
#endif /* _WIN32 */
        pipe_line_free(line);
    }

    line = pipe_line_argv(args);
    if (line)
    {
        printf("argv: %s\n", line);
        pipe_line_free(line);
    }

    for (size_t i = 0; i != n; ++i)
    {
        if (pipe_open(ctx + i, path, args, envp, NULL, PIPE_IO))
        {
            fprintf(stderr, "Initialization failure!\n");
        }
    }

    for (size_t i = 0; i != n; ++i)
    {
        pipe_printf(ctx + i, "%i %zu\n", 1, i);
        pipe_flush(ctx + i);
        pipe_wait(ctx + i, 10);
    }

    for (size_t i = 0; i != n; ++i)
    {
        for (int c = pipe_getc(ctx + i); c != EOF; c = pipe_getc(ctx + i))
        {
            fputc(c, stdout);
            if (c == '\n')
            {
                break;
            }
        }
    }

    for (size_t i = 0; i != n; ++i)
    {
        pipe_wait(ctx + i, ULONG_MAX);
        pipe_close(ctx + i);
    }

    free(ctx);

    return EXIT_SUCCESS;
}
