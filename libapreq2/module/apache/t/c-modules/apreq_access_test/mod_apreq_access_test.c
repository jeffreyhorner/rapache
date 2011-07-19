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

<Location /apreq_access_test>
   TestAccess test
   SetHandler apreq_request_test
</Location>

#endif
#endif

#define APACHE_HTTPD_TEST_ACCESS_CHECKER apreq_access_checker
#define APACHE_HTTPD_TEST_COMMANDS       access_cmds
#define APACHE_HTTPD_TEST_PER_DIR_CREATE create_access_config 

#include "apache_httpd_test.h"
#include "apreq_module_apache2.h"
#include "httpd.h"
#include "apr_strings.h"

extern module AP_MODULE_DECLARE_DATA apreq_access_test_module;

struct access_test_cfg {
    apr_pool_t *pool;
    const char *param;
};

static const char *access_config(cmd_parms *cmd, void *dv, const char *arg)
{
    struct access_test_cfg *cfg = (struct access_test_cfg *)dv;
    cfg->param = apr_pstrdup(cfg->pool, arg);
    return NULL;
}

static const command_rec access_cmds[] =
{
    AP_INIT_TAKE1("TestAccess", access_config, NULL, OR_LIMIT, "'param'"),
    { NULL }
};

static void *create_access_config(apr_pool_t *p, char *dummy)
{
    struct access_test_cfg *cfg = apr_palloc(p, sizeof *cfg);
    cfg->pool = p;
    cfg->param = dummy;
    return cfg;
}

static int apreq_access_checker(request_rec *r)
{
    apreq_handle_t *handle;
    apreq_param_t *param;
    struct access_test_cfg *cfg = (struct access_test_cfg *)
        ap_get_module_config(r->per_dir_config, &apreq_access_test_module);

    if (!cfg || !cfg->param)
        return DECLINED;

    handle = apreq_handle_apache2(r);
    param = apreq_param(handle, cfg->param);
    if (param != NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS,
                      r, "ACCESS GRANTED: %s => %s", cfg->param, param->v.data);
        return OK;
    }
    else {
        const apr_table_t *t = apreq_params(handle, r->pool);
        if (t != NULL) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, r, 
                          "%s not found: parsing error detected (%d params)",
                          cfg->param, apr_table_elts(t)->nelts);
        }
        else {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, r, 
                          "%s not found: paring error detected (no param table)",
                          cfg->param);
        }
        return HTTP_FORBIDDEN;
    }
}

APACHE_HTTPD_TEST_MODULE(apreq_access_test);
