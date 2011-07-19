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
#include "apreq_cookie.h"
#include "apreq_error.h"
#include "apreq_module.h"
#include "apreq_util.h"
#include "at.h"

static const char nscookies[] = "a=1; foo=bar; fl=left; fr=right;bad; "
                                "ns=foo=1&bar=2,frl=right-left; "
                                "flr=left-right; fll=left-left; "
                                "good_one=1;=;bad";

static const char rfccookies[] = "$Version=1; first=a;$domain=quux;second=be,"
                                 "$Version=1;third=cie";

static apr_table_t *jar, *jar2;
static apr_pool_t *p;

static void jar_make(dAT)
{
    jar = apr_table_make(p, APREQ_DEFAULT_NELTS);
    AT_not_null(jar);
    AT_int_eq(apreq_parse_cookie_header(p, jar, nscookies), APREQ_ERROR_NOTOKEN);
    jar2 = apr_table_make(p, APREQ_DEFAULT_NELTS);
    AT_not_null(jar2);
    AT_int_eq(apreq_parse_cookie_header(p, jar2, rfccookies), APR_SUCCESS);
}

static void jar_get_rfc(dAT)
{
    const char *val;
    AT_not_null(val = apr_table_get(jar2, "first"));
    AT_str_eq(val, "a");
    AT_not_null(val = apr_table_get(jar2, "second"));
    AT_str_eq(val, "be");
    AT_not_null(val = apr_table_get(jar2, "third"));
    AT_str_eq(val, "cie");
}

static void jar_get_ns(dAT)
{

    AT_str_eq(apr_table_get(jar, "a"), "1");

    /* ignore wacky cookies that don't have an '=' sign */
    AT_is_null(apr_table_get(jar, "bad"));

    /* accept wacky cookies that contain multiple '=' */
    AT_str_eq(apr_table_get(jar, "ns"), "foo=1&bar=2");

    AT_str_eq(apr_table_get(jar,"foo"), "bar");
    AT_str_eq(apr_table_get(jar,"fl"),  "left");
    AT_str_eq(apr_table_get(jar,"fr"),  "right");
    AT_str_eq(apr_table_get(jar,"frl"), "right-left");
    AT_str_eq(apr_table_get(jar,"flr"), "left-right");
    AT_str_eq(apr_table_get(jar,"fll"), "left-left");
    AT_is_null(apr_table_get(jar,""));
}


static void netscape_cookie(dAT)
{
    char expires[APR_RFC822_DATE_LEN];
    char *val;
    apreq_cookie_t *c;

    *(const char **)&val = apr_table_get(jar, "foo");
    AT_not_null(val);

    c = apreq_value_to_cookie(val);

    AT_str_eq(c->v.data, "bar");
    AT_int_eq(apreq_cookie_version(c), 0);
    AT_str_eq(apreq_cookie_as_string(c, p), "foo=bar");

    c->domain = apr_pstrdup(p, "example.com");
    AT_str_eq(apreq_cookie_as_string(c, p), "foo=bar; domain=example.com");

    c->path = apr_pstrdup(p, "/quux");
    AT_str_eq(apreq_cookie_as_string(c, p), 
              "foo=bar; path=/quux; domain=example.com");

    apreq_cookie_expires(c, "+1y");
    apr_rfc822_date(expires, apr_time_now()
                             + apr_time_from_sec(apreq_atoi64t("+1y")));
    expires[7] = '-';
    expires[11] = '-';
    val = apr_pstrcat(p, "foo=bar; path=/quux; domain=example.com; expires=", 
                      expires, NULL);

    AT_str_eq(apreq_cookie_as_string(c, p), val);
}


static void rfc_cookie(dAT)
{
    apreq_cookie_t *c = apreq_cookie_make(p,"rfc",3,"out",3);
    const char *expected;
    long expires; 

    AT_str_eq(c->v.data, "out");

    apreq_cookie_version_set(c, 1);
    AT_int_eq(apreq_cookie_version(c), 1);
    AT_str_eq(apreq_cookie_as_string(c,p),"rfc=out; Version=1");

    c->domain = apr_pstrdup(p, "example.com");

#ifndef WIN32

    AT_str_eq(apreq_cookie_as_string(c,p),
              "rfc=out; Version=1; domain=\"example.com\"");
    c->path = apr_pstrdup(p, "/quux");
    AT_str_eq(apreq_cookie_as_string(c,p),
              "rfc=out; Version=1; path=\"/quux\"; domain=\"example.com\"");

    apreq_cookie_expires(c, "+3m");
    expires = apreq_atoi64t("+3m");
    expected = apr_psprintf(p, "rfc=out; Version=1; path=\"/quux\"; "
                       "domain=\"example.com\"; max-age=%ld",
                       expires);
    AT_str_eq(apreq_cookie_as_string(c,p), expected);

#else

    expected = "rfc=out; Version=1; domain=\"example.com\"";
    AT_str_eq(apreq_cookie_as_string(c,p), expected);

    c->path = apr_pstrdup(p, "/quux");
    expected = "rfc=out; Version=1; path=\"/quux\"; domain=\"example.com\"";
    AT_str_eq(apreq_cookie_as_string(c,p), expected);

    apreq_cookie_expires(c, "+3m");
    expires = apreq_atoi64t("+3m");
    expected = apr_psprintf(p, "rfc=out; Version=1; path=\"/quux\"; "
                           "domain=\"example.com\"; max-age=%ld",
                           expires);
    AT_str_eq(apreq_cookie_as_string(c,p), expected);

#endif

}


#define dT(func, plan) #func, func, plan


int main(int argc, char *argv[])
{
    unsigned i, plan = 0;
    dAT;
    at_test_t test_list [] = {
        { dT(jar_make, 4) },
        { dT(jar_get_rfc, 6), "1 3 5" },
        { dT(jar_get_ns, 10) },
        { dT(netscape_cookie, 7) },
        { dT(rfc_cookie, 6) },
    };

    apr_initialize();
    atexit(apr_terminate);

    apr_pool_create(&p, NULL);

    AT = at_create(p, 0, at_report_stdout_make(p)); 

    for (i = 0; i < sizeof(test_list) / sizeof(at_test_t);  ++i)
        plan += test_list[i].plan;

    AT_begin(plan);

    for (i = 0; i < sizeof(test_list) / sizeof(at_test_t);  ++i)
        AT_run(&test_list[i]);

    AT_end();

    return 0;
}
