/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define MAXLN 200
#define ISSPACE " \t\n\r\f\v\""

size_t strcspn(const char *p, const char *s) {
    int i, j;

    for (i = 0; p[i]; i++) {
        for (j = 0; s[j]; j++) {
            if (s[j] == p[i]) {
                break;
            }
        }
        if (s[j]) {
            break;
        }
    }
    return (i);
}

char * _getbase(char *p, int *basep) {
    if (p[0] == '0') {
        switch (p[1]) {
        case 'x':
            *basep = 16;
            break;
        case 't':
        case 'n':
            *basep = 10;
            break;
        case 'o':
            *basep = 8;
            break;
        default:
            *basep = 10;
            return (p);
        }
        return (p + 2);
    }
    *basep = 10;
    return (p);
}

/*
 *  _atob(vp,p,base)
 */
int _atob(uint32_t *vp, char *p, int base) {
    uint32_t value, v1, v2;
    char *q, tmp[20];
    int digit;

    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        base = 16;
        p += 2;
    }

    if (base == 16 && (q = strchr(p, '.')) != 0) {
        if (q - p > sizeof(tmp) - 1) {
            return (0);
        }

        strncpy(tmp, p, q - p);
        tmp[q - p] = '\0';
        if (!_atob(&v1, tmp, 16)) {
            return (0);
        }

        q++;
        if (strchr(q, '.')) {
            return (0);
        }

        if (!_atob(&v2, q, 16)) {
            return (0);
        }
        *vp = (v1 << 16) + v2;
        return (1);
    }

    value = *vp = 0;
    for (; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } else {
            return (0);
        }

        if (digit >= base) {
            return (0);
        }
        value *= base;
        value += digit;
    }
    *vp = value;
    return (1);
}

/*
 *  atob(vp,p,base)
 *      converts p to binary result in vp, rtn 1 on success
 */
int atob(uint32_t *vp, char *p, int base) {
    uint32_t v;
    if (base == 0) {
        p = _getbase(p, &base);
    }
    if (_atob(&v, p, base)) {
        *vp = v;
        return (1);
    }
    return (0);
}

/*
 *  vsscanf(buf,fmt,ap)
 */

int vsscanf(const char *buf, const char *s, va_list ap) {
    uint32_t             count, noassign, width, base, lflag;
    const char     *tc;
    char           *t, tmp[MAXLN];

    count = noassign = width = lflag = 0;
    while (*s && *buf) {
        while (isspace(*s)) {
            s++;
        }
        if (*s == '%') {
            s++;
            for (; *s; s++) {
                if (strchr("dibouxcsefg%", *s)) {
                    break;
                }
                if (*s == '*') {
                    noassign = 1;
                } else if (*s == 'l' || *s == 'L') {
                    lflag = 1;
                } else if (*s >= '1' && *s <= '9') {
                    for (tc = s; isdigit(*s); s++);
                    strncpy(tmp, tc, s - tc);
                    tmp[s - tc] = '\0';
                    atob(&width, tmp, 10);
                    s--;
                }
            }
            if (*s == 's') {
                while (isspace(*buf)) {
                    buf++;
                }
                if (!width) {
                    width = strcspn(buf, ISSPACE);
                }
                if (!noassign) {
                    strncpy(t = va_arg(ap, char *), buf, width);
                    t[width] = '\0';
                }
                buf += width;
            } else if (*s == 'c') {
                if (!width) {
                    width = 1;
                }
                if (!noassign) {
                    strncpy(t = va_arg(ap, char *), buf, width);
                    t[width] = '\0';
                }
                buf += width;
            } else if (strchr("dobxu", *s)) {
                while (isspace(*buf)) {
                    buf++;
                }
                if (*s == 'd' || *s == 'u') {
                    base = 10;
                } else if (*s == 'x') {
                    base = 16;
                } else if (*s == 'o') {
                    base = 8;
                } else if (*s == 'b') {
                    base = 2;
                }
                if (!width) {
                    if (isspace(*(s + 1)) || *(s + 1) == 0) {
                        width = strcspn(buf, ISSPACE);
                    } else {
                        width = strchr(buf, *(s + 1)) - buf;
                    }
                }
                strncpy(tmp, buf, width);
                tmp[width] = '\0';
                buf += width;
                if (!noassign) {
                    atob(va_arg(ap, uint32_t *), tmp, base);
                }
            }
            if (!noassign) {
                count++;
            }
            width = noassign = lflag = 0;
            s++;
        } else {
            while (isspace(*buf)) {
                buf++;
            }
            if (*s != *buf) {
                break;
            } else {
                s++, buf++;
            }
        }
    }
    return (count);
}
