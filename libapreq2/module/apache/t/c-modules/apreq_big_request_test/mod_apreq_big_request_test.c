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

<Location /apreq_big_request_test>
   SetHandler apreq_big_request_test
</Location>

#endif
#endif

#define APACHE_HTTPD_TEST_HANDLER apreq_big_request_test_handler

#include "apache_httpd_test.h"
#include "apreq_module_apache2.h"
#include "httpd.h"

static int dump_table(void *count, const char *key, const char *value)
{
    int *c = (int *) count;
    *c = *c + strlen(key) + strlen(value);
    return 1;
}

static int apreq_big_request_test_handler(request_rec *r)
{
    apreq_handle_t *req;
    apr_table_t *params;
    int count = 0;

    if (strcmp(r->handler, "apreq_big_request_test") != 0)
        return DECLINED;

    req = apreq_handle_apache2(r);

    params = apreq_params(req, r->pool);
    apr_table_do(dump_table, &count, params, NULL);
    ap_set_content_type(r, "text/plain");
    ap_rprintf(r, "%d", count);
    return OK;
}

APACHE_HTTPD_TEST_MODULE(apreq_big_request_test);
