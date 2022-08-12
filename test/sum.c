/*!
 @file sum.c
 @copyright Copyright (C) 2020-present tqfx, All rights reserved.
*/

#include <stdio.h>
#if defined(_WIN32)
#include <Windows.h>
#else /* !_WIN32 */
#include <unistd.h>
#endif /* _WIN32 */

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
