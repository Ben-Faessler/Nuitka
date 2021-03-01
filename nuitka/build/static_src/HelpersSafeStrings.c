//     Copyright 2021, Kay Hayen, mailto:kay.hayen@gmail.com
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//
/* This helpers are used to interact safely with buffers to not overflow.

   Currently this is used for char and wchar_t string buffers and shared
   between onefile bootstrap for Windows, plugins and Nuitka core, but
   should not use any Python level functionality.
*/

// This file is included from another C file, help IDEs to still parse it on
// its own.
#ifdef __IDE_ONLY__
#include "windows.h"
#include <stdbool.h>
#endif

#if defined(_WIN32)
#include <Shlwapi.h>
#endif

void copyStringSafe(char *buffer, char const *source, size_t buffer_size) {
    if (strlen(source) >= buffer_size) {
        abort();
    }
    strcpy(buffer, source);
}

void copyStringSafeN(char *buffer, char const *source, size_t n, size_t buffer_size) {
    if (n >= buffer_size - 1) {
        abort();
    }
    strncpy(buffer, source, n);
    buffer[n] = 0;
}

void appendStringSafe(char *buffer, char const *source, size_t buffer_size) {
    if (strlen(source) + strlen(buffer) >= buffer_size) {
        abort();
    }
    strcat(buffer, source);
}

void appendCharSafe(char *buffer, char c, size_t buffer_size) {
    char source[2] = {c, 0};

    appendStringSafe(buffer, source, buffer_size);
}

void appendWStringSafeW(wchar_t *target, wchar_t const *source, size_t buffer_size) {
    while (*target != 0) {
        target++;
        buffer_size -= 1;
    }

    while (*source != 0) {
        if (buffer_size < 1) {
            abort();
        }

        *target++ = *source++;
        buffer_size -= 1;
    }

    *target = 0;
}

void appendCharSafeW(wchar_t *target, char c, size_t buffer_size) {
    while (*target != 0) {
        target++;
        buffer_size -= 1;
    }

    if (buffer_size < 1) {
        abort();
    }

    target += wcslen(target);
    char buffer_c[2] = {c, 0};
    size_t res = mbstowcs(target, buffer_c, 2);
    assert(res == 1);
}

void appendStringSafeW(wchar_t *target, char const *source, size_t buffer_size) {
    while (*target != 0) {
        target++;
        buffer_size -= 1;
    }

    while (*source != 0) {
        appendCharSafeW(target, *source, buffer_size);
        source++;
        buffer_size -= 1;
    }
}

#if defined(_WIN32)
bool expandWindowsPath(wchar_t *target, wchar_t const *source, size_t buffer_size) {
    wchar_t var_name[1024];
    wchar_t *w = NULL;

    while (*source != 0) {
        if (*source == '%') {
            if (w == NULL) {
                w = var_name;
                *w = 0;

                source++;

                continue;
            } else {
                *w = 0;
                _putws(var_name);

                if (wcscmp(var_name, L"TEMP") == 0) {
                    GetTempPathW((DWORD)buffer_size, target);

                    while (*target) {
                        target++;
                        buffer_size -= 1;
                    }
                } else if (wcscmp(var_name, L"PROGRAM") == 0) {
                    if (!GetModuleFileNameW(NULL, target, (DWORD)buffer_size)) {
                        return false;
                    }

                    while (*target) {
                        target++;
                        buffer_size -= 1;
                    }
                } else {
                    return false;
                }

                w = NULL;
                source++;

                continue;
            }
        }

        if (w != NULL) {
            *w++ = *source++;
            continue;
        }

        if (buffer_size < 1) {
            return false;
        }

        *target++ = *source++;
        buffer_size -= 1;
    }

    *target = 0;

    return true;
}
#endif
