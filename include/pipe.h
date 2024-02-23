/*!
 @file pipe.h
 @brief pipeline implementation
*/

#ifndef PIPE_H
#define PIPE_H

#include <stddef.h>
#if defined(_WIN32)
#include <Windows.h>
#else /* !_WIN32 */
#include <sys/types.h>
#endif /* _WIN32 */
#include <stdio.h>

/* attribute visibility */
#if defined(_WIN32) || defined(__CYGWIN__)
#define PIPE_EXPORT __declspec(dllexport)
#define PIPE_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
#define PIPE_EXPORT __attribute__((visibility("default")))
#define PIPE_IMPORT __attribute__((visibility("default")))
#else /* !visibility */
#define PIPE_EXPORT
#define PIPE_IMPORT
#endif /* visibility */
#if defined(PIPE_EXPORTS)
#define PIPE_PUBLIC PIPE_EXPORT
#elif defined(PIPE_IMPORTS)
#define PIPE_PUBLIC PIPE_IMPORT
#else /* !PIPE_PUBLIC */
#define PIPE_PUBLIC
#endif /* PIPE_PUBLIC */

/*!
 @brief instance structure for pipeline
*/
typedef struct pipe_t
{
    FILE *in; //!< only write
    FILE *out; //!< only read
    FILE *err; //!< only read
#if defined(_WIN32)
    PROCESS_INFORMATION _pid;
    HANDLE _in; //!< only write
    HANDLE _out; //!< only read
    HANDLE _err; //!< only read
#else /* !_WIN32 */
    pid_t _pid;
    int _in; //!< only write
    int _out; //!< only read
    int _err; //!< only read
#endif /* _WIN32 */
} pipe_t;

typedef enum pipe_e
{
    PIPE_IN = (1 << 0), //!< stdin
    PIPE_OUT = (1 << 1), //!< stdout
    PIPE_IO = PIPE_IN | PIPE_OUT,
    PIPE_ERR = (1 << 2), //!< stderr
    PIPE_STD = PIPE_IO | PIPE_ERR
} pipe_e;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 @brief initialize an instance of pipeline structure
 @param[in,out] ctx points to an instance of pipeline structure
 @param[in] path the filename associated with the file being executed
 @code{.c}
 char const *path = "/bin/ls";
 @endcode
 @param[in] argv a pointer to the argument block terminated by a NULL pointer
 @code{.c}
 char *argv[] = {"ls", "-hlA", NULL};
 @endcode
 @param[in] envp a pointer to the environment block terminated by a NULL pointer
 @code{.c}
 char *envp[] = {"PATH=/bin", NULL};
 @endcode
 @param[in] cwd if NULL, cwd is current process working directory
 @param[in] std standard input, standard output, standard error
 @return the execution state of the function
  @retval ~0 failure
  @retval 0 success
*/
PIPE_PUBLIC int pipe_open(pipe_t *ctx, char const *path, char *const argv[], char *const envp[], char const *cwd, int std);

/*!
 @brief terminate an instance of pipeline structure
 @param[in] ctx points to an instance of pipeline structure
 @return the execution state of the function
  @retval ~0 failure
  @retval 0 success
*/
PIPE_PUBLIC int pipe_close(pipe_t *ctx);

/*!
 @brief pending within the timeout period specified synchronously
 @param[in] ctx points to an instance of pipeline structure
 @param[in] ms timeout period millisecond specified
 @return the execution state of the function
  @retval ~0 failure
  @retval 0 success
*/
PIPE_PUBLIC int pipe_wait(pipe_t const *ctx, unsigned long ms);

/*!
 @brief convert argv to line, "argv1" "argv2" ... "argvn"
 @param[in] argv a pointer to the argument block terminated by a NULL pointer
*/
PIPE_PUBLIC char *pipe_line_argv(char *const argv[]);

/*!
 @brief convert envp to line
  unix: var1=value1:var2=value2\0
  windows: var1=value1\0var2=value2\0\0
 @param[in] envp a pointer to the environment block terminated by a NULL pointer
*/
PIPE_PUBLIC char *pipe_line_envp(char *const envp[]);

/*!
 @brief release memory for pipe line
 @param[in] line return value of pipe_line_argv or pipe_line_envp
*/
PIPE_PUBLIC void pipe_line_free(void *line);

/*!
 @brief access standard stream mode for pipeline
 @param[in] ctx points to an instance of pipeline structure
*/
PIPE_PUBLIC int pipe_mode(pipe_t const *ctx);

/*!
 @brief check if pipeline is valid
 @param[in] ctx points to an instance of pipeline structure
  @retval 0 invalid
  @retval !0 valid
*/
PIPE_PUBLIC int pipe_valid(pipe_t const *ctx);

/*!
 @brief flush standard stream for pipeline
 @param[in] ctx points to an instance of pipeline structure
*/
PIPE_PUBLIC int pipe_flush(pipe_t const *ctx);

/*!
 @brief read a block of the standard output of the pipeline
 @param[in] ctx points to an instance of pipeline structure
 @param data address of a buffer
 @param byte length of a buffer
 @return size of the read data
*/
PIPE_PUBLIC size_t pipe_read(pipe_t const *ctx, void *data, size_t byte);

/*!
 @brief read a block of the standard error output of the pipeline
 @param[in] ctx points to an instance of pipeline structure
 @param data address of a buffer
 @param byte length of a buffer
 @return size of the read data
*/
PIPE_PUBLIC size_t pipe_reade(pipe_t const *ctx, void *data, size_t byte);

/*!
 @brief write a block of the standard input to the pipeline
 @param[in] ctx points to an instance of pipeline structure
 @param data address of a buffer to write to the pipeline
 @param byte length of a buffer to write to the pipeline
 @return size of the written data
*/
PIPE_PUBLIC size_t pipe_write(pipe_t const *ctx, void const *data, size_t byte);

/*!
 @brief access a stream pointer to the standard input of the pipeline
 @param[in] ctx points to an instance of pipeline structure
*/
PIPE_PUBLIC FILE *pipe_stdin(pipe_t *ctx);

/*!
 @brief access a stream pointer to the standard output of the pipeline
 @param[in] ctx points to an instance of pipeline structure
*/
PIPE_PUBLIC FILE *pipe_stdout(pipe_t *ctx);

/*!
 @brief access a stream pointer to the standard error output of the pipeline
 @param[in] ctx points to an instance of pipeline structure
*/
PIPE_PUBLIC FILE *pipe_stderr(pipe_t *ctx);

#if defined(__cplusplus)
} /* extern "C" */
#endif /* __cplusplus */

#endif /* pipe.h */
