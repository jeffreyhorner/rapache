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

<Location /apreq_request_test>
   APREQ2_ReadLimit 500K
   SetHandler apreq_request_test
</Location>

#endif
#endif

#define APACHE_HTTPD_TEST_HANDLER apreq_request_test_handler

#include "apache_httpd_test.h"
#include "apreq_module_apache2.h"
#include "httpd.h"

static int dump_table(void *ctx, const char *key, const char *value)
{
    request_rec *r = ctx;
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS,
                  r, "%s => %s", key, value);
    ap_rprintf(r, "\t%s => %s\n", key, value);
    return 1;
}

static int apreq_request_test_handler(request_rec *r)
{
    apreq_handle_t *req;
    const apr_table_t *t;
    apr_status_t s;

    if (strcmp(r->handler, "apreq_request_test") != 0)
        return DECLINED;

    req = apreq_handle_apache2(r);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS,
                  r, "starting apreq_request_test");

    s = ap_discard_request_body(r);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, s,
                  r, "discard request body");

    ap_set_content_type(r, "text/plain");
    ap_rputs("ARGS:\n",r);
    if (apreq_args(req, &t) == APR_SUCCESS)
        apr_table_do(dump_table, r, t, NULL);

    if (apreq_body(req, &t) == APR_SUCCESS) {
        ap_rputs("BODY:\n",r);
        apr_table_do(dump_table, r, t, NULL);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS,
                  r, "finished apreq_request_test");

    return OK;
}

APACHE_HTTPD_TEST_MODULE(apreq_request_test);
