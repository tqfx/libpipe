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
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int pipe_valid(const pipe_s *ctx)
{
#if defined(_WIN32)
    return ctx->pid != NULL;
#else /* !_WIN32 */
    return ctx->pid > 0;
#endif /* _WIN32 */
}

int pipe_flush(const pipe_s *ctx)
{
    int ok = 0;
    if (ctx->in)
    {
        ok += fflush(ctx->in);
    }
    if (ctx->out)
    {
        ok += fflush(ctx->out);
    }
    if (ctx->err)
    {
        ok += fflush(ctx->err);
    }
    return ok;
}

int pipe_mode(const pipe_s *ctx)
{
    int status = 0;
    if (ctx->in)
    {
        status |= PIPE_IN;
    }
    if (ctx->out)
    {
        status |= PIPE_OUT;
    }
    if (ctx->err)
    {
        status |= PIPE_ERR;
    }
    return status;
}

int pipe_getc(const pipe_s *ctx)
{
    return fgetc(ctx->out);
}

int pipe_gete(const pipe_s *ctx)
{
    return fgetc(ctx->err);
}

int pipe_putc(const pipe_s *ctx, int c)
{
    return fputc(c, ctx->in);
}

int pipe_puts(const pipe_s *ctx, const char *str)
{
    return fputs(str, ctx->in);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif /* __GNUC__ || __clang__ */

int pipe_scanf(const pipe_s *ctx, const char *fmt, ...)
{
    int stats;
    va_list va;
    va_start(va, fmt);
    stats = vfscanf(ctx->out, fmt, va);
    va_end(va);
    return stats;
}

int pipe_scanfe(const pipe_s *ctx, const char *fmt, ...)
{
    int stats;
    va_list va;
    va_start(va, fmt);
    stats = vfscanf(ctx->err, fmt, va);
    va_end(va);
    return stats;
}

int pipe_printf(const pipe_s *ctx, const char *fmt, ...)
{
    int stats;
    va_list va;
    va_start(va, fmt);
    stats = vfprintf(ctx->in, fmt, va);
    va_end(va);
    return stats;
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif /* __GNUC__ || __clang__ */

size_t pipe_read(const pipe_s *ctx, void *data, size_t byte)
{
    return fread(data, 1, byte, ctx->out);
}

size_t pipe_reade(const pipe_s *ctx, void *data, size_t byte)
{
    return fread(data, 1, byte, ctx->err);
}

size_t pipe_write(const pipe_s *ctx, const void *data, size_t byte)
{
    return fwrite(data, 1, byte, ctx->in);
}

char *pipe_line_argv(char *const argv[])
{
    size_t cur = 0;
    size_t size = 0;
    char *line = NULL;

    if (argv == NULL || *argv == NULL)
    {
        goto exit;
    }

    for (char *const *strv = argv; *strv; ++strv)
    {
        for (const char *str = *strv; *str; ++str)
        {
            if (*str != '"')
            {
                ++size;
            }
            else
            {
                /* \" */
                size += 2;
            }
        }
        /* " " */
        size += 3;
    }

#if defined(_WIN32)
    line = (char *)LocalAlloc(0, size);
#else /* !_WIN32 */
    line = (char *)malloc(size);
#endif /* _WIN32 */
    if (line == NULL)
    {
        goto exit;
    }

    for (char *const *strv = argv; *strv; ++strv)
    {
        line[cur++] = '"';
        for (const char *str = *strv; *str; ++str)
        {
            if (*str != '"')
            {
                line[cur++] = *str;
            }
            else
            {
                line[cur++] = '\\';
                line[cur++] = *str;
            }
        }
        line[cur++] = '"';
        line[cur++] = ' ';
    }
    line[cur - 1] = 0;

exit:
    return line;
}

void pipe_line_free(void *line)
{
#if defined(_WIN32)
    LocalFree(line);
#else /* !_WIN32 */
    free(line);
#endif /* _WIN32 */
}

#if defined(_WIN32)

#include <io.h>
#include <strsafe.h>

char *pipe_line_envp(char *const envp[])
{
    size_t cur = 0;
    size_t size = 1;
    char *line = NULL;

    if (envp == NULL || *envp == NULL)
    {
        goto exit;
    }

    for (char *const *strv = envp; *strv; ++strv)
    {
        size += (size_t)lstrlen(*strv) + 1;
    }

    line = (char *)LocalAlloc(0, size);
    if (line == NULL)
    {
        goto exit;
    }

    for (char *const *strv = envp; *strv; ++strv)
    {
        if (FAILED(StringCchCopy(line + cur, size - cur, *strv)))
        {
            goto exit;
        }
        cur += (size_t)lstrlen(*strv) + 1;
    }
    line[cur] = 0;

exit:
    return line;
}

int pipe_open(pipe_s *ctx, const char *path, char *const argv[], char *const envp[], const char *cwd, int std)
{
    int ok = ~0;
    ctx->in = NULL;
    ctx->out = NULL;
    ctx->err = NULL;

    // Set up members of the PROCESS_INFORMATION structure.
    ctx->pid = (PROCESS_INFORMATION *)LocalAlloc(LMEM_ZEROINIT, sizeof(PROCESS_INFORMATION));

    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES saAttr = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = TRUE,
    };

    HANDLE hChildStdinRd = GetStdHandle(STD_INPUT_HANDLE);
    if (std & PIPE_IN)
    {
        HANDLE hChildStdinWr = NULL;
        // Create a pipe for the child process's STDIN.
        CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0);
        // Ensure the write handle to the pipe for STDIN is not inherited.
        SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0);
        ctx->in = _fdopen(_open_osfhandle((intptr_t)hChildStdinWr, O_BINARY), "w");
    }

    HANDLE hChildStdoutWr = GetStdHandle(STD_OUTPUT_HANDLE);
    if (std & PIPE_OUT)
    {
        HANDLE hChildStdoutRd = NULL;
        // Create a pipe for the child process's STDOUT.
        CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0);
        // Ensure the read handle to the pipe for STDOUT is not inherited.
        SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
        ctx->out = _fdopen(_open_osfhandle((intptr_t)hChildStdoutRd, O_BINARY), "r");
    }

    HANDLE hChildStderrWr = GetStdHandle(STD_ERROR_HANDLE);
    if (std & PIPE_ERR)
    {
        HANDLE hChildStderrRd = NULL;
        // Create a pipe for the child process's STDERR.
        CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0);
        // Ensure the read handle to the pipe for STDERR is not inherited.
        SetHandleInformation(hChildStderrRd, HANDLE_FLAG_INHERIT, 0);
        ctx->err = _fdopen(_open_osfhandle((intptr_t)hChildStderrRd, O_BINARY), "r");
    }

    // Set up members of the STARTUPINFO structure.
    STARTUPINFO siStartInfo = {0};
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdError = hChildStderrWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Convert argv and envp to line
    LPSTR Argv = pipe_line_argv(argv);
    LPSTR Envp = pipe_line_envp(envp);

    // Create the child process.
    if (CreateProcess(path,
                      Argv,
                      NULL,
                      NULL,
                      TRUE,
                      0,
                      Envp,
                      cwd,
                      &siStartInfo,
                      ctx->pid))
    {
        // Close handles to its primary thread.
        CloseHandle(ctx->pid->hThread);
        ok = 0;
    }

    // Free line for argv and envp
    pipe_line_free(Argv);
    pipe_line_free(Envp);

    if (std & PIPE_IN)
    {
        // Close handles to the stdin pipes.
        CloseHandle(hChildStdinRd);
    }
    if (std & PIPE_OUT)
    {
        // Close handles to the stdout pipes.
        CloseHandle(hChildStdoutWr);
    }
    if (std & PIPE_ERR)
    {
        // Close handles to the stderr pipes.
        CloseHandle(hChildStderrWr);
    }

    return ok;
}

int pipe_close(pipe_s *ctx)
{
    DWORD status = 0;

    if (ctx->pid == NULL)
    {
        errno = ECHILD;
        return ~0;
    }

    if (ctx->in && fclose(ctx->in) == EOF)
    {
        clearerr(ctx->in);
    }
    ctx->in = NULL;

    if (ctx->out && fclose(ctx->out) == EOF)
    {
        clearerr(ctx->out);
    }
    ctx->out = NULL;

    if (ctx->err && fclose(ctx->err) == EOF)
    {
        clearerr(ctx->err);
    }
    ctx->err = NULL;

    CloseHandle(ctx->pid->hProcess);

    GetExitCodeProcess(ctx->pid->hProcess, &status);

    LocalFree(ctx->pid);
    ctx->pid = NULL;

    return (int)status;
}

int pipe_wait(const pipe_s *ctx, unsigned long ms)
{
    switch (WaitForSingleObject(ctx->pid->hProcess, ms ? ms : INFINITE))
    {
    case WAIT_OBJECT_0:
        return 0;
    case WAIT_TIMEOUT:
        errno = ETIMEDOUT;
        return ~0;
    default:
        return ~1;
    }
}

#else /* !_WIN32 */

#include <unistd.h>
#include <sys/wait.h>

char *pipe_line_envp(char *const envp[])
{
    size_t cur = 0;
    size_t size = 0;
    char *line = NULL;

    if (envp == NULL || *envp == NULL)
    {
        goto exit;
    }

    for (char *const *strv = envp; *strv; ++strv)
    {
        size += strlen(*strv) + 1;
    }

    line = (char *)malloc(size);
    if (line == NULL)
    {
        goto exit;
    }

    for (char *const *strv = envp; *strv; ++strv, ++cur)
    {
        size = strlen(*strv);
        memcpy(line + cur, *strv, size);
        cur = cur + size;
        line[cur] = ':';
    }
    line[cur - 1] = 0;

exit:
    return line;
}

#define R 0
#define W 1

int pipe_open(pipe_s *ctx, const char *path, char *const argv[], char *const envp[], const char *cwd, int std)
{
    ctx->in = 0;
    ctx->out = 0;
    ctx->err = 0;
    ctx->pid = ~0;

    /* create stdin pipes */
    int pipe_in[2];
    if (std & PIPE_IN)
    {
        if (pipe(pipe_in) < 0)
        {
            goto pipe_in;
        }
    }
    /* create stdout pipes */
    int pipe_out[2];
    if (std & PIPE_OUT)
    {
        if (pipe(pipe_out) < 0)
        {
            goto pipe_out;
        }
    }
    /* create stderr pipes */
    int pipe_err[2];
    if (std & PIPE_ERR)
    {
        if (pipe(pipe_err) < 0)
        {
            goto pipe_err;
        }
    }

    /* create a child process */
    ctx->pid = fork();
    if (ctx->pid < 0)
    {
        goto pipe_std;
    }

    if (ctx->pid == 0)
    {
        if (cwd)
        {
            chdir(cwd);
        }
        if (std & PIPE_IN)
        {
            close(pipe_in[W]);
            if (pipe_in[R] != STDIN_FILENO)
            {
                int ok = dup2(pipe_in[R], STDIN_FILENO);
                close(pipe_in[R]);
                if (ok < 0)
                {
                    _exit(EXIT_FAILURE);
                }
            }
        }
        if (std & PIPE_OUT)
        {
            close(pipe_out[R]);
            if (pipe_out[W] != STDOUT_FILENO)
            {
                int ok = dup2(pipe_out[W], STDOUT_FILENO);
                close(pipe_out[W]);
                if (ok < 0)
                {
                    _exit(EXIT_FAILURE);
                }
            }
        }
        if (std & PIPE_ERR)
        {
            close(pipe_err[R]);
            if (pipe_err[W] != STDERR_FILENO)
            {
                int ok = dup2(pipe_err[W], STDERR_FILENO);
                close(pipe_err[W]);
                if (ok < 0)
                {
                    _exit(EXIT_FAILURE);
                }
            }
        }

        execve(path, argv, envp ? envp : environ);

        _exit(127); /* command not found */
    }

    if (std & PIPE_IN)
    {
        ctx->in = fdopen(pipe_in[W], "w");
        close(pipe_in[R]);
    }
    if (std & PIPE_OUT)
    {
        ctx->out = fdopen(pipe_out[R], "r");
        close(pipe_out[W]);
    }
    if (std & PIPE_ERR)
    {
        ctx->err = fdopen(pipe_err[R], "r");
        close(pipe_err[W]);
    }

    return 0;

pipe_std:
    if (std & PIPE_ERR)
    {
        close(pipe_err[R]);
        close(pipe_err[W]);
    }
pipe_err:
    if (std & PIPE_OUT)
    {
        close(pipe_out[R]);
        close(pipe_out[W]);
    }
pipe_out:
    if (std & PIPE_IN)
    {
        close(pipe_in[R]);
        close(pipe_in[W]);
    }
pipe_in:
    return ~0;
}

#undef R
#undef W

int pipe_close(pipe_s *ctx)
{
    int status = 0;

    if (ctx->pid < 0)
    {
        errno = ECHILD;
        return ~0;
    }

    if (ctx->in && fclose(ctx->in) == EOF)
    {
        clearerr(ctx->in);
    }
    ctx->in = 0;
    if (ctx->out && fclose(ctx->out) == EOF)
    {
        clearerr(ctx->out);
    }
    ctx->out = 0;
    if (ctx->err && fclose(ctx->err) == EOF)
    {
        clearerr(ctx->err);
    }
    ctx->err = 0;

    /* check if the child process to terminate */
    while (waitpid(ctx->pid, &status, WNOHANG) == 0)
    {
        kill(ctx->pid, SIGTERM);
    }
    ctx->pid = ~0;

    /* check if the child process terminated normally */
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    /* check if the child process was terminated by a signal */
    if (WIFSIGNALED(status))
    {
        return WTERMSIG(status);
    }
    /* reached only if the process did not terminate normally */
    errno = ECHILD;
    return status;
}

int pipe_wait(const pipe_s *ctx, unsigned long ms)
{
    errno = 0;
    int status = 0;
    pid_t wait = waitpid(ctx->pid, &status, WNOHANG);
    if (wait != 0 || ms == 0)
    {
        /* wait for the child process to terminate */
        waitpid(ctx->pid, &status, 0);
        goto exit;
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    sigset_t orig_mask;
    if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0)
    {
        return ~0;
    }

    struct timespec timeout = {
        .tv_sec = (time_t)(ms / 1000),
        .tv_nsec = (time_t)(ms % 1000) * 1000000,
    };
    while (sigtimedwait(&mask, NULL, &timeout) < 0)
    {
        if (errno == EAGAIN)
        {
            errno = ETIMEDOUT;
            return ~0;
        }
    }

    if (sigprocmask(SIG_SETMASK, &orig_mask, 0) < 0)
    {
        return ~0;
    }

exit:
    return status;
}

#endif /* _WIN32 */
