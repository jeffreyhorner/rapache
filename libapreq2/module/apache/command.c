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

#include "apr_strings.h"

#include "apreq_util.h"
#include "apreq_module_apache.h"

#include "httpd.h"
#include "http_log.h"
#include "http_request.h"

#include "apreq_private_apache.h"

/**
 * <h2>Server Configuration Directives</h2>
 *
 * <TABLE class="qref"><CAPTION>Per-directory commands for mod_apreq</CAPTION>
 * <TR><TH>Directive</TH><TH>Context</TH><TH>Default</TH><TH>Description</TH></TR>
 * <TR class="odd"><TD>APREQ_ReadLimit</TD><TD>directory</TD><TD>-1 (Unlimited)</TD><TD>
 * Maximum number of bytes mod_apreq will send off to libapreq for parsing.  
 * mod_apreq will log this event and remove itself from the filter chain.
 * The APREQ_ERROR_GENERAL error will be reported to libapreq2 users via the return 
 * value of apreq_env_read().
 * </TD></TR>
 * <TR><TD>APREQ_BrigadeLimit</TD><TD>directory</TD><TD> #APREQ_MAX_BRIGADE_LEN </TD><TD>
 * Maximum number of bytes apreq will allow to accumulate
 * within a brigade.  Excess data will be spooled to a
 * file bucket appended to the brigade.
 * </TD></TR>
 * <TR class="odd"><TD>APREQ_TempDir</TD><TD>directory</TD><TD>NULL</TD><TD>
 * Sets the location of the temporary directory apreq will use to spool
 * overflow brigade data (based on the APREQ_BrigadeLimit setting).
 * If left unset, libapreq2 will select a platform-specific location via apr_temp_dir_get().
 * </TD></TR>
 * </TABLE>
 *
 * <h2>Implementation Details</h2>
 * <pre>
 * XXX apreq as a normal input filter
 * XXX apreq as a "virtual" content handler.
 * XXX apreq as a transparent "tee".
 * XXX apreq parser registration in post_config
 * </pre>
 * @{
 */


static void *apreq_create_dir_config(pool *p, char *d)
{
    /* d == OR_ALL */
    struct dir_config *dc = ap_palloc(p, sizeof *dc);
    dc->temp_dir      = NULL;
    dc->read_limit    = (apr_uint64_t)-1;
    dc->brigade_limit = APREQ_DEFAULT_BRIGADE_LIMIT;
    return dc;
}

static void *apreq_merge_dir_config(pool *p, void *a_, void *b_)
{
    struct dir_config *a = a_, *b = b_, *c = ap_palloc(p, sizeof *c);

    c->temp_dir      = (b->temp_dir != NULL)            /* overrides ok */
                      ? b->temp_dir : a->temp_dir;

    c->brigade_limit = (b->brigade_limit == (apr_size_t)-1) /* overrides ok */
                      ? a->brigade_limit : b->brigade_limit;

    c->read_limit    = (b->read_limit < a->read_limit)  /* why min? */
                      ? b->read_limit : a->read_limit;

    return c;
}

static const char *apreq_set_temp_dir(cmd_parms *cmd, void *data,
                                      const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->temp_dir = arg;
    return NULL;
}

static const char *apreq_set_read_limit(cmd_parms *cmd, void *data,
                                        const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->read_limit = apreq_atoi64f(arg);
    return NULL;
}

static const char *apreq_set_brigade_limit(cmd_parms *cmd, void *data,
                                           const char *arg)
{
    struct dir_config *conf = data;
    const char *err = ap_check_cmd_context(cmd, NOT_IN_LIMIT);

    if (err != NULL)
        return err;

    conf->brigade_limit = apreq_atoi64f(arg);
    return NULL;
}


static const command_rec apreq_cmds[] =
{
    { "APREQ_TempDir", apreq_set_temp_dir, NULL, OR_ALL, TAKE1,
      "Default location of temporary directory" },
    { "APREQ_ReadLimit", apreq_set_read_limit, NULL, OR_ALL, TAKE1,
      "Maximum amount of data that will be fed into a parser." },
    { "APREQ_BrigadeLimit", apreq_set_brigade_limit, NULL, OR_ALL, TAKE1,
      "Maximum in-memory bytes a brigade may use." },
    { NULL }
};

static void apreq_cleanup(void *data)
{
    apr_pool_t *p = data;
    apr_pool_destroy(p);
    apr_terminate();
}

static void apreq_init (server_rec *s, pool *sp)
{
    /* APR_HOOK_FIRST because we want other modules to be able to
       register parsers in their post_config hook */

    apr_pool_t *p;
    apr_initialize();
    apr_pool_create(&p, NULL);
    apreq_initialize(p);
    ap_register_cleanup(sp, p, apreq_cleanup, apreq_cleanup);
}



/** @} */


module MODULE_VAR_EXPORT apreq_module =
{
    STANDARD_MODULE_STUFF,
    apreq_init,                /* module initializer                 */
    apreq_create_dir_config,   /* per-directory config creator       */
    apreq_merge_dir_config,    /* dir config merger                  */
    NULL,                      /* server config creator              */
    NULL,                      /* server config merger               */
    apreq_cmds,                /* command table                      */
    NULL,                      /* [9]  content handlers              */
    NULL,                      /* [2]  URI-to-filename translation   */
    NULL,                      /* [5]  check/validate user_id        */
    NULL,                      /* [6]  check user_id is valid *here* */
    NULL,                      /* [4]  check access by host address  */
    NULL,                      /* [7]  MIME type checker/setter      */
    NULL,                      /* [8]  fixups                        */
    NULL,                      /* [10] logger                        */
    NULL,                      /* [3]  header parser                 */
    NULL,                      /* process initialization             */
    NULL,                      /* process exit/cleanup               */
    NULL                       /* [1]  post read_request handling    */
};

