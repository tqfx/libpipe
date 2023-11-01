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
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#include <strsafe.h>
#else /* !_WIN32 */
#include <unistd.h>
#include <sys/wait.h>
#endif /* _WIN32 */

int pipe_mode(pipe_s const *ctx)
{
    int status = 0;
    if (ctx->_in)
    {
        status |= PIPE_IN;
    }
    if (ctx->_out)
    {
        status |= PIPE_OUT;
    }
    if (ctx->_err)
    {
        status |= PIPE_ERR;
    }
    return status;
}

int pipe_valid(pipe_s const *ctx)
{
#if defined(_WIN32)
    return ctx->_pid.hProcess != NULL;
#else /* !_WIN32 */
    return ctx->_pid > ~0;
#endif /* _WIN32 */
}

int pipe_flush(pipe_s const *ctx)
{
    int ok = 0;
#if defined(_WIN32)
    if (ctx->_in)
    {
        FlushFileBuffers(ctx->_in);
    }
    if (ctx->_out)
    {
        FlushFileBuffers(ctx->_out);
    }
    if (ctx->_err)
    {
        FlushFileBuffers(ctx->_err);
    }
#else /* !_WIN32 */
    if (ctx->_in > ~0)
    {
        fsync(ctx->_in);
    }
    if (ctx->_out > ~0)
    {
        fsync(ctx->_out);
    }
    if (ctx->_err > ~0)
    {
        fsync(ctx->_err);
    }
#endif /* _WIN32 */
    return ok;
}

size_t pipe_read(pipe_s const *ctx, void *data, size_t byte)
{
#if defined(_WIN32)
    DWORD size = 0;
    ReadFile(ctx->_out, data, byte, &size, NULL);
    return (size_t)size;
#else /* !_WIN32 */
    ssize_t size = read(ctx->_out, data, byte);
    return size > ~0 ? (size_t)size : 0;
#endif /* _WIN32 */
}

size_t pipe_reade(pipe_s const *ctx, void *data, size_t byte)
{
#if defined(_WIN32)
    DWORD size = 0;
    ReadFile(ctx->_err, data, byte, &size, NULL);
    return (size_t)size;
#else /* !_WIN32 */
    ssize_t size = read(ctx->_err, data, byte);
    return size > ~0 ? (size_t)size : 0;
#endif /* _WIN32 */
}

size_t pipe_write(pipe_s const *ctx, void const *data, size_t byte)
{
#if defined(_WIN32)
    DWORD size = 0;
    WriteFile(ctx->_in, data, byte, &size, NULL);
    return (size_t)size;
#else /* !_WIN32 */
    ssize_t size = write(ctx->_in, data, byte);
    return size > ~0 ? (size_t)size : 0;
#endif /* _WIN32 */
}

FILE *pipe_stdin(pipe_s *ctx)
{
    if (ctx->in == NULL)
    {
#if defined(_WIN32)
        ctx->in = _fdopen(_open_osfhandle((intptr_t)ctx->_in, _O_BINARY | _O_WRONLY), "wb");
#else /* !_WIN32 */
        ctx->in = fdopen(ctx->_in, "wb");
#endif /* _WIN32 */
    }
    return ctx->in;
}

FILE *pipe_stdout(pipe_s *ctx)
{
    if (ctx->out == NULL)
    {
#if defined(_WIN32)
        ctx->out = _fdopen(_open_osfhandle((intptr_t)ctx->_out, _O_BINARY | _O_RDONLY), "rb");
#else /* !_WIN32 */
        ctx->out = fdopen(ctx->_out, "rb");
#endif /* _WIN32 */
    }
    return ctx->out;
}

FILE *pipe_stderr(pipe_s *ctx)
{
    if (ctx->err == NULL)
    {
#if defined(_WIN32)
        ctx->err = _fdopen(_open_osfhandle((intptr_t)ctx->_err, _O_BINARY | _O_RDONLY), "rb");
#else /* !_WIN32 */
        ctx->err = fdopen(ctx->_err, "rb");
#endif /* _WIN32 */
    }
    return ctx->err;
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
        for (char const *str = *strv; *str; ++str)
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
        for (char const *str = *strv; *str; ++str)
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

char *pipe_line_envp(char *const envp[])
{
    char *line = NULL;
    size_t cur = 0;
#if defined(_WIN32)
    size_t size = 1;

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
#else /* !_WIN32 */
    size_t size = 0;

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
#endif /* _WIN32 */
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

int pipe_open(pipe_s *ctx, char const *path, char *const argv[], char *const envp[], char const *cwd, int std)
{
#if defined(_WIN32)
    int ok = ~0;

    // Set up members of the pipe_s structure.
    memset(ctx, 0, sizeof(*ctx));

    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES saAttr = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = TRUE,
    };

    HANDLE hChildStdinRd = GetStdHandle(STD_INPUT_HANDLE);
    if (std & PIPE_IN)
    {
        // Create a pipe for the child process's STDIN.
        CreatePipe(&hChildStdinRd, &ctx->_in, &saAttr, 0);
        // Ensure the write handle to the pipe for STDIN is not inherited.
        SetHandleInformation(ctx->_in, HANDLE_FLAG_INHERIT, 0);
    }

    HANDLE hChildStdoutWr = GetStdHandle(STD_OUTPUT_HANDLE);
    if (std & PIPE_OUT)
    {
        // Create a pipe for the child process's STDOUT.
        CreatePipe(&ctx->_out, &hChildStdoutWr, &saAttr, 0);
        // Ensure the read handle to the pipe for STDOUT is not inherited.
        SetHandleInformation(ctx->_out, HANDLE_FLAG_INHERIT, 0);
    }

    HANDLE hChildStderrWr = GetStdHandle(STD_ERROR_HANDLE);
    if (std & PIPE_ERR)
    {
        // Create a pipe for the child process's STDERR.
        CreatePipe(&ctx->_err, &hChildStderrWr, &saAttr, 0);
        // Ensure the read handle to the pipe for STDERR is not inherited.
        SetHandleInformation(ctx->_err, HANDLE_FLAG_INHERIT, 0);
    }

    // Set up members of the STARTUPINFO structure.
    STARTUPINFO siStartInfo = {0};
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdError = hChildStderrWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Convert argv and envp to line
    LPSTR args = pipe_line_argv(argv);
    LPSTR envs = pipe_line_envp(envp);

    // Create the child process.
    if (CreateProcess(path,
                      args,
                      NULL,
                      NULL,
                      TRUE,
                      0,
                      envs,
                      cwd,
                      &siStartInfo,
                      &ctx->_pid))
    {
        // Close handles to its primary thread.
        CloseHandle(ctx->_pid.hThread);
        ctx->_pid.hThread = NULL;
        ctx->_pid.dwThreadId = 0;
        ok = 0;
    }

    // Free line for argv and envp
    pipe_line_free(args);
    pipe_line_free(envs);

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
#else /* !_WIN32 */
#define R 0
#define W 1
    ctx->_in = ~0;
    ctx->_out = ~0;
    ctx->_err = ~0;
    ctx->_pid = ~0;
    ctx->in = NULL;
    ctx->out = NULL;
    ctx->err = NULL;

    /* check execute permission */
    if (cwd && access(cwd, X_OK))
    {
        goto pipe_in;
    }

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
    ctx->_pid = fork();
    if (ctx->_pid < 0)
    {
        goto pipe_std;
    }

    if (ctx->_pid == 0)
    {
        if (cwd)
        {
            chdir(cwd);
        }
        if (std & PIPE_IN)
        {
            close(pipe_in[W]);
            int ok = dup2(pipe_in[R], STDIN_FILENO);
            if (pipe_in[R] != STDIN_FILENO)
            {
                close(pipe_in[R]);
            }
            if (ok < 0)
            {
                _exit(EXIT_FAILURE);
            }
        }
        if (std & PIPE_OUT)
        {
            close(pipe_out[R]);
            int ok = dup2(pipe_out[W], STDOUT_FILENO);
            if (pipe_out[W] != STDOUT_FILENO)
            {
                close(pipe_out[W]);
            }
            if (ok < 0)
            {
                _exit(EXIT_FAILURE);
            }
        }
        if (std & PIPE_ERR)
        {
            close(pipe_err[R]);
            int ok = dup2(pipe_err[W], STDERR_FILENO);
            if (pipe_err[W] != STDERR_FILENO)
            {
                close(pipe_err[W]);
            }
            if (ok < 0)
            {
                _exit(EXIT_FAILURE);
            }
        }

        execve(path, argv, envp ? envp : environ);

        _exit(127); /* command not found */
    }

    if (std & PIPE_IN)
    {
        ctx->_in = pipe_in[W];
        close(pipe_in[R]);
    }
    if (std & PIPE_OUT)
    {
        ctx->_out = pipe_out[R];
        close(pipe_out[W]);
    }
    if (std & PIPE_ERR)
    {
        ctx->_err = pipe_err[R];
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
#undef R
#undef W
#endif /* _WIN32 */
}

int pipe_close(pipe_s *ctx)
{
#if defined(_WIN32)
    DWORD status = 0;

    if (ctx->_pid.hProcess == NULL)
    {
        errno = ECHILD;
        return ~0;
    }

    if (ctx->_in)
    {
        CloseHandle(ctx->_in);
        ctx->_in = NULL;
    }
    if (ctx->_out)
    {
        CloseHandle(ctx->_out);
        ctx->_out = NULL;
    }
    if (ctx->_err)
    {
        CloseHandle(ctx->_err);
        ctx->_err = NULL;
    }

    CloseHandle(ctx->_pid.hProcess);
    GetExitCodeProcess(ctx->_pid.hProcess, &status);
    ctx->_pid.hProcess = NULL;
    ctx->_pid.dwProcessId = 0;
    return (int)status;
#else /* !_WIN32 */
    int status = 0;

    if (ctx->_pid < 0)
    {
        errno = ECHILD;
        return ~0;
    }

    if (ctx->_in > ~0)
    {
        close(ctx->_in);
        ctx->_in = ~0;
    }
    if (ctx->_out > ~0)
    {
        close(ctx->_out);
        ctx->_out = ~0;
    }
    if (ctx->_err > ~0)
    {
        close(ctx->_err);
        ctx->_err = ~0;
    }

    /* check if the child process to terminate */
    while (waitpid(ctx->_pid, &status, WNOHANG) == 0)
    {
        kill(ctx->_pid, SIGTERM);
    }
    ctx->_pid = ~0;

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
#endif /* _WIN32 */
}

int pipe_wait(pipe_s const *ctx, unsigned long ms)
{
#if defined(_WIN32)
    switch (WaitForSingleObject(ctx->_pid.hProcess, ms ? ms : INFINITE))
    {
    case WAIT_OBJECT_0:
        return 0;
    case WAIT_TIMEOUT:
        errno = ETIMEDOUT;
        return ~0;
    default:
        return ~1;
    }
#else /* !_WIN32 */
    errno = 0;
    int status = 0;
    pid_t wait = waitpid(ctx->_pid, &status, WNOHANG);
    if (wait != 0 || ms == 0)
    {
        /* wait for the child process to terminate */
        waitpid(ctx->_pid, &status, 0);
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
#endif /* _WIN32 */
}
