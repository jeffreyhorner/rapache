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

#ifdef CONFIG_FOR_HTTPD_TEST
#if CONFIG_FOR_HTTPD_TEST

<Location /apreq_cookie_test>
   SetHandler apreq_cookie_test
</Location>

#endif
#endif

#define APACHE_HTTPD_TEST_HANDLER apreq_cookie_test_handler

#include "apache_httpd_test.h"
#include "apreq_module_apache2.h"
#include "apreq_util.h"
#include "httpd.h"
#include <assert.h>


static int dump_table(void *ctx, const char *key, const char *value)
{
    request_rec *r = ctx;
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS,
                  r, "[%s] => [%s]", key, value);
    return 1;
}


static int apreq_cookie_test_handler(request_rec *r)
{
    apreq_handle_t *req;
    apr_status_t s;
    const char *test, *key;
    apreq_cookie_t *cookie;
    apr_size_t size;
    char *dest;
    const apr_table_t *args;

    if (strcmp(r->handler, "apreq_cookie_test") != 0)
        return DECLINED;

    req = apreq_handle_apache2(r);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r, 
                  "starting cookie tests");

    apreq_args(req, &args);

    apr_table_do(dump_table, r, args, NULL);

    test = apr_table_get(args, "test");
    key = apr_table_get(args, "key");

    cookie = apreq_cookie(req, key);

    ap_set_content_type(r, "text/plain");

    if (strcmp(test, "bake") == 0) {
        apreq_cookie_tainted_off(cookie);
        s = apreq_cookie_bake(cookie, req);
    }
    else if (strcmp(test, "bake2") == 0) {
        apreq_cookie_tainted_off(cookie);
        s = apreq_cookie_bake2(cookie, req);
    }
    else {
        size = strlen(cookie->v.data);
        dest = apr_palloc(r->pool, size + 1);
        s = apreq_decode(dest, &size, cookie->v.data, size);
        if (s == APR_SUCCESS)
            ap_rprintf(r, "%s", dest);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, s, r, 
                  "finished cookie tests");

    return OK;
}

APACHE_HTTPD_TEST_MODULE(apreq_cookie_test);
