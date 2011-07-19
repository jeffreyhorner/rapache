/*
**  Copyright 2003-2005  The Apache Software Foundation
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

#include "apreq_module.h"
#include "apreq_util.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_tables.h"
#include <stdlib.h>

static int dump_table(void *count, const char *key, const char *value)
{
    int *c = (int *) count;
    int value_len = key - value - 1; /* == strlen(value) by construction */
    if (value_len) 
        *c += strlen(key) + value_len;
    return 1;
}

#define cookie_bake(c) apr_file_printf(out, "Set-Cookie: %s\n", \
                                          apreq_cookie_as_string(c, pool))

#define cookie_bake2(c) apr_file_printf(out, "Set-Cookie2: %s\n", \
                                          apreq_cookie_as_string(c, pool))


int main(int argc, char const * const * argv)
{
    apr_pool_t *pool;
    apreq_handle_t *req;
    const apreq_param_t *foo, *bar, *test, *key;
    apr_file_t *out;

    atexit(apr_terminate);
    if (apr_app_initialize(&argc, &argv, NULL) != APR_SUCCESS) {
        fprintf(stderr, "apr_app_initialize failed\n");
        exit(-1);
    }

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        fprintf(stderr, "apr_pool_create failed\n");
        exit(-1);
    }

    if (apreq_initialize(pool) != APR_SUCCESS) {
        fprintf(stderr, "apreq_initialize failed\n");
        exit(-1);
    }

    req = apreq_handle_cgi(pool);

    apr_file_open_stdout(&out, pool);

    foo = apreq_param(req, "foo");
    bar = apreq_param(req, "bar");

    test = apreq_param(req, "test");
    key  = apreq_param(req, "key");

    if (foo || bar) {
        apr_file_printf(out, "%s", "Content-Type: text/plain\n\n");

        if (foo) {
            apr_file_printf(out, "\t%s => %s\n", "foo", foo->v.data);
        }
        if (bar) {
            apr_file_printf(out, "\t%s => %s\n", "bar", bar->v.data);
        }
    }
    
    else if (test && key) {
        apreq_cookie_t *cookie;
        char *dest;
        apr_size_t dlen;

        cookie = apreq_cookie(req, key->v.data);

        if (cookie == NULL) {
            exit(-1);
        }

        if (strcmp(test->v.data, "bake") == 0) {
            cookie_bake(cookie);
        }
        else if (strcmp(test->v.data, "bake2") == 0) {
            cookie_bake2(cookie);
        }

        apr_file_printf(out, "%s", "Content-Type: text/plain\n\n");
        dest = apr_pcalloc(pool, cookie->v.dlen + 1);
        if (apreq_decode(dest, &dlen, cookie->v.data, 
                         cookie->v.dlen) == APR_SUCCESS)
            apr_file_printf(out, "%s", dest);
        else {
            exit(-1);
        }
    }

    else { 
        const apr_table_t *params = apreq_params(req, pool);
        int count = 0;
        apr_file_printf(out, "%s", "Content-Type: text/plain\n\n");

        if (params == NULL) {
            exit(-1);
        }
        apr_table_do(dump_table, &count, params, NULL);
        apr_file_printf(out, "%d", count);
    }

    return 0;
}
