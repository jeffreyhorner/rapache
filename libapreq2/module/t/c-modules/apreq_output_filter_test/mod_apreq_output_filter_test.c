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

<Location />
   AddOutputFilter APREQ_OUTPUT_FILTER html
</Location>

#endif
#endif

#include "apache_httpd_test.h"
#include "apreq_module_apache2.h"
#include "httpd.h"
#include "util_filter.h"

static const char filter_name[] =  "APREQ_OUTPUT_FILTER";
extern module AP_MODULE_DECLARE_DATA apreq_output_filter_test_module;

static apr_status_t apreq_output_filter_test_init(ap_filter_t *f)
{
    apreq_handle_t *handle;
    handle = apreq_handle_apache2(f->r);
    return APR_SUCCESS;
}

struct ctx_t {
    request_rec *r;
    apr_bucket_brigade *bb;
};

static int dump_table(void *data, const char *key, const char *value)
{
    struct ctx_t *ctx = (struct ctx_t *)data;
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, ctx->r, 
                  "%s => %s", key, value);
    apr_brigade_printf(ctx->bb,NULL,NULL,"\t%s => %s\n", key, value);
    return 1;
}

static apr_status_t apreq_output_filter_test(ap_filter_t *f, apr_bucket_brigade *bb)
{
    request_rec *r = f->r;
    apreq_handle_t *handle;
    apr_bucket_brigade *eos;
    struct ctx_t ctx = {r, bb};
    const apr_table_t *t;

    if (!APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(bb)))
        return ap_pass_brigade(f->next,bb);

    eos = apr_brigade_split(bb, APR_BRIGADE_LAST(bb));

    handle = apreq_handle_apache2(r);
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r, 
                  "appending parsed data");

    apr_brigade_puts(bb, NULL, NULL, "\n--APREQ OUTPUT FILTER--\nARGS:\n");

    apreq_args(handle, &t);
    if (t != NULL)
        apr_table_do(dump_table, &ctx, t, NULL);

    apreq_body(handle, &t);
    if (t != NULL) {
        apr_brigade_puts(bb,NULL,NULL,"BODY:\n");
        apr_table_do(dump_table, &ctx, t, NULL);
    }
    APR_BRIGADE_CONCAT(bb,eos);
    return ap_pass_brigade(f->next,bb);
}

static void register_hooks (apr_pool_t *p)
{
    (void)p;
    ap_register_output_filter(filter_name, apreq_output_filter_test, 
                              apreq_output_filter_test_init,
                              AP_FTYPE_CONTENT_SET);
}

module AP_MODULE_DECLARE_DATA apreq_output_filter_test_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks,			/* callback for registering hooks */
};
