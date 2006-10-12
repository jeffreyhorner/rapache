/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 2000-2006   The R Development Core Team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/* Taken from  R 2.4.0 */

typedef struct Rconn  *Rconnection;
struct Rconn {
    char* class;
    char* description;
    char mode[5];
    Rboolean text, isopen, incomplete, canread, canwrite, canseek, blocking, 
	isGzcon;
    Rboolean (*open)(struct Rconn *);
    void (*close)(struct Rconn *); /* routine closing after auto open */
    void (*destroy)(struct Rconn *); /* when closing connection */
    int (*vfprintf)(struct Rconn *, const char *, va_list);
    int (*fgetc)(struct Rconn *);
    int (*fgetc_internal)(struct Rconn *);
/*    int (*ungetc)(int c, struct Rconn *); */
    double (*seek)(struct Rconn *, double, int, int);
    void (*truncate)(struct Rconn *);
    int (*fflush)(struct Rconn *);
    size_t (*read)(void *, size_t, size_t, struct Rconn *);
    size_t (*write)(const void *, size_t, size_t, struct Rconn *);
/*    void (*onerror)(struct Rconn *); */
    int nPushBack, posPushBack; /* number of lines, position on top line */
    char **PushBack;
    int save, save2;
    /* unsigned char encoding[256];*/
    char encname[101];
    /* will be iconv_t, which is a pointer. NULL if not in use */
    void *inconv, *outconv;
    /* The idea here is that no MBCS char will ever not fit */
    char iconvbuff[25], oconvbuff[50], *next, init_out[25];
    short navail, inavail;
    Rboolean EOF_signalled;
    void *private;
};

Rconnection getConnection(int n);


