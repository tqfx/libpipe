# pipe {#mainpage}

```c
#include "pipe.h"

int main(int argc, char *argv[])
{
#if defined(_WIN32)
    const char *path = NULL;
#else /* !_WIN32 */
    const char *path = "/bin/ls";
#endif /* _WIN32 */
    char *args[] = {"ls", "-Ahl", "--color", NULL};
    const char *cwd = argc > 1 ? argv[argc - 1] : NULL;

    pipe_s ctx[1];

    if (pipe_open(ctx, path, args, NULL, cwd, PIPE_OUT))
    {
        fprintf(stderr, "Initialization failure!\n");
    }

    if (pipe_mode(ctx) & PIPE_OUT)
    {
        char buffer[BUFSIZ];
        size_t n = pipe_read(ctx, buffer, BUFSIZ);
        while (n)
        {
            printf("%.*s", (int)n, buffer);
            n = pipe_read(ctx, buffer, BUFSIZ);
        }
    }

    pipe_wait(ctx, 0);
    return pipe_close(ctx);
}
```

## Copyright {#copyright}

Copyright (C) 2020-present tqfx, All rights reserved.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at <https://mozilla.org/MPL/2.0/>.
