/*
**  Copyright 2004-2005  The Apache Software Foundation
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include "apr_file_io.h"
#include "at.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_tables.h"
#include "apr_env.h"
#include <assert.h>


apr_status_t at_begin(at_t *t, int total)
{
    char buf[32];
    apr_snprintf(buf, 32, "1..%d", total);
    return at_report(t, buf);
}

static apr_status_t test_cleanup(void *data)
{
    at_t *t = data;
    if (t->current < t->plan)
        return at_report(t, "Bail out!");
    else
        return at_report(t, "END"); 
}

void at_end(at_t *t)
{     
    apr_pool_cleanup_kill(t->pool, t, test_cleanup);
    test_cleanup(t);
}

apr_status_t at_comment(at_t *t, const char *fmt, va_list vp)
{
    apr_status_t s;
    char buf[256], *b = buf + 2;
    char *end;
    int rv;
    rv = apr_vsnprintf(b, 250, fmt, vp);

    if (rv <= 0)
        return APR_EINVAL;


    end = b + rv;

    buf[0] = '#';
    buf[1] = ' ';

    if (rv == 250) {
        end[-1] = '.';
        *end++ = '.';
        *end++ = '.';
        *end++ = '\n';
        *end = 0;
    }
    else if (end[-1] != '\n') {
        *end++ = '\n';
        *end = 0;
    }

    b = buf;
    while (1) {
        char *eol;

        eol = strchr(b + 2, '\n');
        *eol = 0;
        s = at_report(t, b);
        if (s != APR_SUCCESS || eol == end - 1)
            break;

        b    = eol - 1;
        b[0] = '#';
        b[1] = ' ';
    }

    return s;
} 

void at_ok(at_t *t, int is_ok, const char *label, const char *file, int line)
{
    char format[] = "not ok %d - %s # %s (%s:%d) test %d in %s";
    char *end = format + 10;
    char *fmt = is_ok ? format + 4 : format;
    char buf[256];
    const char *comment = NULL;
    int rv, is_fatal = 0, is_skip = 0, is_todo = 0;

    t->current++;
      
    if (*t->fatal == t->current) {
        t->fatal++;
        is_fatal = 1;
    }

    if (*t->skip == t->current) {
        t->skip++;
        is_skip = 1;
    }

    if (*t->todo == t->current) {
        t->todo++;
        is_todo = 1;
    }

    if (AT_FLAG_CONCISE(t->flags))
        format[9] = '\0';
    else if (is_ok && !AT_FLAG_TRACE(t->flags))
        format[14] = '\0';
    else if (is_fatal && ! is_ok)
        comment = "fatal";
    else
        comment = is_todo ? "todo" : is_skip ? "skip" : "at";

    rv = apr_snprintf(buf, 256, fmt, t->current + t->prior, 
                      label, comment,  file, line, t->current, t->name);

    if (rv <= 0)
        exit(-1);

    end = buf + rv;

    if (rv == 250) {
        *end++ = '.';
        *end++ = '.';
        *end++ = '.';
        *end = '\0';
    }

    if (memchr(buf, '\n', rv) != NULL || at_report(t, buf) != APR_SUCCESS)
        exit(-1);

    if (!is_ok && is_fatal) {
        while (t->current++ < t->plan) {
            apr_snprintf(buf, 256, "not ok %d # skipped: aborting test %s",
                     t->prior + t->current, t->name);
            at_report(t, buf);
        }
        longjmp(*t->abort, 0);
    }
}

struct at_report_file {
    at_report_t module;
    apr_file_t *file;
};


static apr_status_t at_report_file_write(at_report_t *ctx, const char *msg)
{
    struct at_report_file *r = (struct at_report_file *)ctx;
    apr_file_t *f = r->file;
    apr_size_t len = strlen(msg);
    apr_status_t s;

    s = apr_file_write_full(f, msg, len, &len);
    if (s != APR_SUCCESS)
        return s;

    s = apr_file_putc('\n', f);
    if (s != APR_SUCCESS)
        return s;

    return apr_file_flush(f);
}

at_report_t *at_report_file_make(apr_pool_t *p, apr_file_t *f)
{
    struct at_report_file *r = apr_palloc(p, sizeof *r);
    r->module.func = at_report_file_write;
    r->file = f;
    return &r->module;
}



struct at_report_local {
    at_report_t  module;
    at_t        *t;
    at_report_t *saved_report;
    const int   *saved_fatal;
    int          dummy_fatal;
    const char  *file;
    int          line;
    int          passed;
    apr_pool_t  *pool;
};


static apr_status_t report_local_cleanup(void *data)
{
    struct at_report_local *q = data;
    dAT = q->t;
    char label[32];

    apr_snprintf(label, 32, "collected %d passing tests", q->passed);

    AT->report = q->saved_report;
    AT->fatal = q->saved_fatal;

    at_ok(q->t, 1, label, q->file, q->line);
    return APR_SUCCESS;
}


static apr_status_t at_report_local_write(at_report_t *ctx, const char *msg)
{
    char buf[256];
    struct at_report_local *q = (struct at_report_local *)ctx;
    dAT = q->t;

    if (strncmp(msg, "not ok", 6) == 0) {
        q->saved_report->func(q->saved_report, msg);
        AT->report = q->saved_report;
        AT->fatal = q->saved_fatal;
        apr_pool_cleanup_kill(q->pool, q, report_local_cleanup);
        while (AT->current++ < AT->plan) {
            apr_snprintf(buf, 256, "not ok %d # skipped: aborting test %s",
                     AT->prior + AT->current, AT->name);
            at_report(AT, buf);
        }
        longjmp(*AT->abort, 0);
    }
    else if (strncmp(msg, "ok", 2) == 0) {
        AT->current--;
        q->passed++;
    }
    return APR_SUCCESS;
}

void at_report_local(at_t *AT, apr_pool_t *p, const char *file, int line)
{
    struct at_report_local *q = apr_palloc(p, sizeof *q);
    q->module.func = at_report_local_write;
    q->t = AT;
    q->saved_report = AT->report;
    q->saved_fatal = AT->fatal;
    q->dummy_fatal = 0;
    q->file = apr_pstrdup(p, file);
    q->line = line;
    q->passed = 0;
    q->pool = p;

    AT->fatal = &q->dummy_fatal;
    AT->report = &q->module;

    if (*q->saved_fatal == AT->current + 1)
        q->saved_fatal++;

    apr_pool_cleanup_register(p, q, report_local_cleanup, 
                                    report_local_cleanup);
}


at_t *at_create(apr_pool_t *pool, unsigned char flags, at_report_t *report)
{
    at_t *t = apr_pcalloc(pool, sizeof *t);
    t->flags = flags;
    t->report = report;
    t->pool = pool;

    apr_pool_cleanup_register(pool, t, test_cleanup, test_cleanup);
    return t;
}


#define AT_NELTS 4

static int* at_list(apr_pool_t *pool, const char *spec, int *list)
{
    apr_array_header_t arr;
    int prev, current = 0;

    arr.pool = pool;
    arr.elt_size = sizeof *list;
    arr.nelts = 0;
    arr.nalloc = AT_NELTS;
    arr.elts = (char *)list;

    do {
        while (*spec && !apr_isdigit(*spec))
            ++spec;

        prev = current;
        current = (int)apr_strtoi64(spec, (char **)(void *)&spec, 10);
        *(int *)apr_array_push(&arr) = current;

    } while (prev >= current);

    return (int *)arr.elts;
}



apr_status_t at_run(at_t *AT, const at_test_t *test)
{
    int dummy = 0, fbuf[AT_NELTS], sbuf[AT_NELTS], tbuf[AT_NELTS];
    jmp_buf j;

    AT->current = 0;
    AT->prior   += AT->plan;
    AT->name    = test->name;
    AT->plan    = test->plan;

    if (test->fatals)
        AT->fatal = at_list(AT->pool, test->fatals, fbuf);
    else
        AT->fatal = &dummy;

    if (test->skips)
        AT->skip = at_list(AT->pool, test->skips, sbuf);
    else
        AT->skip = &dummy;

    if (test->todos)
        AT->todo = at_list(AT->pool, test->todos, tbuf);
    else
        AT->todo = &dummy;

    AT->abort = &j;
    if (setjmp(j) == 0) {
        test->func(AT);
        return APR_SUCCESS;
    }
    AT->abort = NULL;
    return APR_EGENERAL;
}
