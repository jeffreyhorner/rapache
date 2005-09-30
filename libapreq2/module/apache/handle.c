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
#include "apreq_module_apache.h"

#include "http_main.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

#include "apreq_private_apache.h"

/* This is basically the cgi_handle struct with a request_rec */

struct apache_handle {
    struct apreq_handle_t        handle;
    request_rec                 *r;
    apr_pool_t                  *pool;
    apr_bucket_alloc_t          *bucket_alloc;

    apr_table_t                 *jar, *args, *body;
    apr_status_t                 jar_status,
                                 args_status,
                                 body_status;

    apreq_parser_t              *parser;
    apreq_hook_t                *hook_queue;

    const char                  *temp_dir;
    apr_size_t                   brigade_limit;
    apr_uint64_t                 read_limit;
    apr_uint64_t                 bytes_read;
    apr_bucket_brigade          *in;

};

static const char *apache_header_in(apreq_handle_t *env, const char *name)
{
    struct apache_handle *req = (struct apache_handle *)env;
    return ap_table_get(req->r->headers_in, name);
}

static apr_status_t apache_header_out(apreq_handle_t *env,
                                       const char *name, char *value)
{
    struct apache_handle *req = (struct apache_handle *)env;
    ap_table_add(req->r->err_headers_out, name, value);
    return APR_SUCCESS;
}

#ifdef APR_POOL_DEBUG
static apr_status_t ba_cleanup(void *data)
{
    apr_bucket_alloc_t *ba = data;
    apr_bucket_alloc_destroy(ba);
    return APR_SUCCESS;
}
#endif

static void init_body(apreq_handle_t *env)
{
    struct apache_handle *req = (struct apache_handle *)env;
    const char *cl_header = apache_header_in(env, "Content-Length");
    apr_bucket_alloc_t *ba = req->bucket_alloc;
    request_rec *r = req->r;

    req->body  = apr_table_make(req->pool, APREQ_DEFAULT_NELTS);
#ifdef APR_POOL_DEBUG
    apr_pool_cleanup_register(req->pool, ba, ba_cleanup, ba_cleanup);
#endif
    if (cl_header != NULL) {
        char *dummy;
        apr_int64_t content_length = apr_strtoi64(cl_header, &dummy, 0);

        if (dummy == NULL || *dummy != 0) {
            req->body_status = APREQ_ERROR_BADHEADER;
            ap_log_rerror(APLOG_MARK, APLOG_ERR, r,
                          "Invalid Content-Length header (%s)", cl_header);
            return;
        }
        else if ((apr_uint64_t)content_length > req->read_limit) {
            req->body_status = APREQ_ERROR_OVERLIMIT;
            ap_log_rerror(APLOG_MARK, APLOG_ERR, r,
                          "Content-Length header (%s) exceeds configured "
                          "max_body limit (%" APR_UINT64_T_FMT ")", 
                          cl_header, req->read_limit);
            return;
        }
    }

    if (req->parser == NULL) {
        const char *ct_header = apache_header_in(env, "Content-Type");

        if (ct_header != NULL) {
            apreq_parser_function_t pf = apreq_parser(ct_header);

            if (pf != NULL) {
                req->parser = apreq_parser_make(req->pool,
                                                ba,
                                                ct_header, 
                                                pf,
                                                req->brigade_limit,
                                                req->temp_dir,
                                                req->hook_queue,
                                                NULL);
            }
            else {
                req->body_status = APREQ_ERROR_NOPARSER;
                return;
            }
        }
        else {
            req->body_status = APREQ_ERROR_NOHEADER;
            return;
        }
    }
    else {
        if (req->parser->brigade_limit > req->brigade_limit)
            req->parser->brigade_limit = req->brigade_limit;
        if (req->temp_dir != NULL)
            req->parser->temp_dir = req->temp_dir;
        if (req->hook_queue != NULL)
            apreq_parser_add_hook(req->parser, req->hook_queue);
    }

    req->hook_queue = NULL;
    req->in         = apr_brigade_create(req->pool, ba);
    req->body_status = APR_INCOMPLETE;

}

static apr_status_t apache_read(apreq_handle_t *env,
                                apr_off_t bytes)
{
    struct apache_handle *req = (struct apache_handle *)env;
    request_rec *r = req->r;
    apr_bucket *e;
    long got = 0;
    char buf[HUGE_STRING_LEN];
    char tc[] = "[apreq] apache_read";
    int want = sizeof buf;

    if (req->body_status == APR_EINIT)
        init_body(env);

    if (req->body_status != APR_INCOMPLETE)
        return req->body_status;

    /* XXX want a loop here, instead of reducing bytes */

    if (bytes > HUGE_STRING_LEN)
        want = HUGE_STRING_LEN;
    else
        want = bytes;


    ap_hard_timeout(tc, r);
    got = ap_get_client_block(r, buf, want);
    ap_kill_timeout(r);

    if (got > 0) {
        e = apr_bucket_transient_create(buf, got, req->bucket_alloc);
        req->bytes_read += got;
    }
    else
        e = apr_bucket_eos_create(req->bucket_alloc);

    APR_BRIGADE_INSERT_TAIL(req->in, e);


    if (req->bytes_read <= req->read_limit) {
        req->body_status = apreq_parser_run(req->parser, req->body, req->in);
        apr_brigade_cleanup(req->in);
    }
    else {
        req->body_status = APREQ_ERROR_OVERLIMIT;
        ap_log_rerror(APLOG_MARK, APLOG_ERR, req->r,
                      "Bytes read (%" APR_UINT64_T_FMT
                      ") exceeds configured limit (%" APR_UINT64_T_FMT ")",
                      req->bytes_read, req->read_limit);
    }

    return req->body_status;
}






static apr_status_t apache_jar(apreq_handle_t *env, const apr_table_t **t)
{
    struct apache_handle *req = (struct apache_handle *)env;
    request_rec *r = req->r;

    if (req->jar_status == APR_EINIT) {
        const char *cookies = ap_table_get(r->headers_in, "Cookie");
        if (cookies != NULL) {
            req->jar = apr_table_make(req->pool, APREQ_DEFAULT_NELTS);
            req->jar_status = 
                apreq_parse_cookie_header(req->pool, req->jar, cookies);
        }
        else
            req->jar_status = APREQ_ERROR_NODATA;
    }

    *t = req->jar;
    return req->jar_status;
}

static apr_status_t apache_args(apreq_handle_t *env, const apr_table_t **t)
{
    struct apache_handle *req = (struct apache_handle *)env;
    request_rec *r = req->r;

    if (req->args_status == APR_EINIT) {
        if (r->args != NULL) {
            req->args = apr_table_make(req->pool, APREQ_DEFAULT_NELTS);
            req->args_status = 
                apreq_parse_query_string(req->pool, req->args, r->args);
        }
        else
            req->args_status = APREQ_ERROR_NODATA;
    }

    *t = req->args;
    return req->args_status;
}




static apreq_cookie_t *apache_jar_get(apreq_handle_t *env, const char *name)
{
    struct apache_handle *req = (struct apache_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (req->jar_status == APR_EINIT)
        apache_jar(env, &t);
    else
        t = req->jar;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_cookie(val);
}

static apreq_param_t *apache_args_get(apreq_handle_t *env, const char *name)
{
    struct apache_handle *req = (struct apache_handle *)env;
    const apr_table_t *t;
    const char *val;

    if (req->args_status == APR_EINIT)
        apache_args(env, &t);
    else
        t = req->args;

    if (t == NULL)
        return NULL;

    val = apr_table_get(t, name);
    if (val == NULL)
        return NULL;

    return apreq_value_to_param(val);
}


static apr_status_t apache_body(apreq_handle_t *env, const apr_table_t **t)
{
    struct apache_handle *req = (struct apache_handle *)env;

    switch (req->body_status) {

    case APR_EINIT:
        init_body(env);
        if (req->body_status != APR_INCOMPLETE)
            break;

    case APR_INCOMPLETE:
        while (apache_read(env, APREQ_DEFAULT_READ_BLOCK_SIZE) == APR_INCOMPLETE)
            ;   /*loop*/
    }

    *t = req->body;
    return req->body_status;
}

static apreq_param_t *apache_body_get(apreq_handle_t *env, const char *name)
{
    struct apache_handle *req = (struct apache_handle *)env;
    const char *val;

    switch (req->body_status) {

    case APR_EINIT:

        init_body(env);
        if (req->body_status != APR_INCOMPLETE)
            return NULL;
        apache_read(env, APREQ_DEFAULT_READ_BLOCK_SIZE);

    case APR_INCOMPLETE:

        val = apr_table_get(req->body, name);
        if (val != NULL)
            return apreq_value_to_param(val);

        do {
            /* riff on Duff's device */
            apache_read(env, APREQ_DEFAULT_READ_BLOCK_SIZE);

    default:

            val = apr_table_get(req->body, name);
            if (val != NULL)
                return apreq_value_to_param(val);

        } while (req->body_status == APR_INCOMPLETE);

    }

    return NULL;
}

static
apr_status_t apache_parser_get(apreq_handle_t *env, 
                                  const apreq_parser_t **parser)
{
    struct apache_handle *req = (struct apache_handle *)env;
    *parser = req->parser;
    return APR_SUCCESS;
}

static
apr_status_t apache_parser_set(apreq_handle_t *env, 
                                apreq_parser_t *parser)
{
    struct apache_handle *req = (struct apache_handle *)env;

    if (req->parser == NULL) {

        if (req->hook_queue != NULL) {
            apr_status_t s = apreq_parser_add_hook(parser, req->hook_queue);
            if (s != APR_SUCCESS)
                return s;
        }
        if (req->temp_dir != NULL) {
            req->temp_dir = req->temp_dir;
        }
        if (req->brigade_limit < req->brigade_limit) {
            req->brigade_limit = req->brigade_limit;
        }

        req->hook_queue = NULL;
        req->parser = parser;
        return APR_SUCCESS;
    }
    else
        return APREQ_ERROR_MISMATCH;
}

static
apr_status_t apache_hook_add(apreq_handle_t *env,
                              apreq_hook_t *hook)
{
    struct apache_handle *req = (struct apache_handle *)env;

    if (req->parser != NULL) {
        return apreq_parser_add_hook(req->parser, hook);
    }
    else if (req->hook_queue != NULL) {
        apreq_hook_t *h = req->hook_queue;
        while (h->next != NULL)
            h = h->next;
        h->next = hook;
    }
    else {
        req->hook_queue = hook;
    }
    return APR_SUCCESS;

}

static
apr_status_t apache_brigade_limit_set(apreq_handle_t *env,
                                       apr_size_t bytes)
{
    struct apache_handle *req = (struct apache_handle *)env;
    apr_size_t *limit = (req->parser == NULL) 
                      ? &req->brigade_limit 
                      : &req->parser->brigade_limit;

    if (*limit > bytes) {
        *limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}

static
apr_status_t apache_brigade_limit_get(apreq_handle_t *env,
                                       apr_size_t *bytes)
{
    struct apache_handle *req = (struct apache_handle *)env;
    *bytes = (req->parser == NULL) 
           ?  req->brigade_limit 
           :  req->parser->brigade_limit;

    return APR_SUCCESS;
}

static
apr_status_t apache_read_limit_set(apreq_handle_t *env,
                                    apr_uint64_t bytes)
{
    struct apache_handle *req = (struct apache_handle *)env;
    if (req->read_limit > bytes && req->bytes_read < bytes) {
        req->read_limit = bytes;
        return APR_SUCCESS;
    }

    return APREQ_ERROR_MISMATCH;
}

static
apr_status_t apache_read_limit_get(apreq_handle_t *env,
                                    apr_uint64_t *bytes)
{
    struct apache_handle *req = (struct apache_handle *)env;
    *bytes = req->read_limit;
    return APR_SUCCESS;
}

static
apr_status_t apache_temp_dir_set(apreq_handle_t *env,
                                  const char *path)
{
    struct apache_handle *req = (struct apache_handle *)env;

    const char **curpath = (req->parser == NULL) 
                         ? &req->temp_dir
                         : &req->parser->temp_dir;

    if (*curpath == NULL) {
        *curpath = apr_pstrdup(req->pool, path);
        return APR_SUCCESS;
    }

    return APREQ_ERROR_NOTEMPTY;
}

static
apr_status_t apache_temp_dir_get(apreq_handle_t *env,
                                  const char **path)
{
    struct apache_handle *req = (struct apache_handle *)env;

    *path = req->parser ? req->parser->temp_dir : req->temp_dir;
    return APR_SUCCESS;
}

static APREQ_MODULE(apache, 20050227);

static void apreq_cleanup(void *data)
{
    struct apache_handle *req = data;
    apr_pool_destroy(req->pool);
}


APREQ_DECLARE(apreq_handle_t *) apreq_handle_apache(request_rec *r)
{
    struct apache_handle *req =
        ap_get_module_config(r->request_config, &apreq_module);
    struct dir_config *d =
        ap_get_module_config(r->per_dir_config, &apreq_module);

    if (req != NULL)
        return &req->handle;

    apr_pool_create(&req->pool, NULL);
    req = apr_pcalloc(req->pool, sizeof *req);
    req->bucket_alloc = apr_bucket_alloc_create(req->pool);

    req->handle.module = &apache_module;
    req->r = r;

    req->args_status = req->jar_status = req->body_status = APR_EINIT;
    ap_register_cleanup(r->pool, req, apreq_cleanup, apreq_cleanup);

    if (d == NULL) {
        req->read_limit    = (apr_uint64_t)-1;
        req->brigade_limit = APREQ_DEFAULT_BRIGADE_LIMIT;
    }
    else {
        req->temp_dir      = d->temp_dir;
        req->read_limit    = d->read_limit;
        req->brigade_limit = d->brigade_limit;
    }

    ap_set_module_config(r->request_config, &apreq_module, req);

    return &req->handle;
}
