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
#if defined(_WIN32)
#include <Windows.h>
#else /* !_WIN32 */
#include <unistd.h>
#endif /* _WIN32 */

int main(int argc, char *argv[])
{
    size_t n = strtoul(argv[argc - 1], NULL, 0);
    if (n == 0)
    {
        unsigned long id = 0, addr = 0;
        for (int i = 0; i != argc; ++i)
        {
            printf("%s ", argv[i]);
        }
        scanf(" %lu%lx", &id, &addr);
        printf("[%lu]=0x%lX\n", id, addr);
#if defined(_WIN32)
        Sleep(100);
#else /* !_WIN32 */
        usleep(100000);
#endif /* _WIN32 */
        return 0;
    }

    pipe_s *ctx = (pipe_s *)malloc(sizeof(pipe_s) * n);
    char *line = pipe_line_envp((char *[]){"/usr/local/bin", "/usr/bin", "/bin", 0});
    char *args[] = {argv[0], NULL};
    char *envp[] = {NULL};
    if (line)
    {
#if defined(_WIN32)
        printf("path: ");
        for (char const *str = line; *str;)
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
        if (pipe_open(ctx + i, argv[0], args, envp, NULL, PIPE_IO))
        {
            fprintf(stderr, "[%zu] Initialization failure!\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (size_t i = 0; i != n; ++i)
    {
        char buffer[BUFSIZ];
        int size = sprintf(buffer, "%zu %p\n", i, pipe_stdout(ctx + i));
        pipe_write(ctx + i, buffer, (size_t)size);
        pipe_flush(ctx + i);
        pipe_wait(ctx + i, 1);
    }

    for (size_t i = 0; i != n; ++i)
    {
        char buffer[BUFSIZ];
        size_t size = pipe_read(ctx + i, buffer, BUFSIZ);
        for (size_t j = 0; buffer[j] != '\n' && j < size; ++j)
        {
            putchar(buffer[j]);
        }
        putchar('\n');
    }

    for (size_t i = 0; i != n; ++i)
    {
        pipe_wait(ctx + i, 0);
        pipe_close(ctx + i);
    }

    free(ctx);

    return EXIT_SUCCESS;
}
