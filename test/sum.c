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
#if defined(_WIN32)
#include <Windows.h>
#else /* !_WIN32 */
#include <unistd.h>
#endif /* _WIN32 */
#include <stdio.h>

int main(int argc, char *argv[])
{
    for (int i = 0; i != argc; ++i)
    {
        printf("%s ", argv[i]);
    }

    int a = 0, b = 0;
    scanf(" %i%i", &a, &b);
    printf("%i + %i = %i\n", a, b, a + b);

#if defined(_WIN32)
    Sleep(100);
#else /* !_WIN32 */
    usleep(100000);
#endif /* _WIN32 */

    return 0;
}
