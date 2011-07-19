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

#ifndef APREQ_XS_TABLES_H
#define APREQ_XS_TABLES_H

/* backward compatibility macros support */

#include "ppport.h"

/**************************************************/


#if (PERL_VERSION >= 8) /* MAGIC ITERATOR REQUIRES 5.8 */

/* Requires perl 5.8 or better. 
 * A custom MGVTBL with its "copy" slot filled allows
 * us to FETCH a table entry immediately during iteration.
 * For multivalued keys this is essential in order to get
 * the value corresponding to the current key, otherwise
 * values() will always report the first value repeatedly.
 * With this MGVTBL the keys() list always matches up with
 * the values() list, even in the multivalued case.
 * We only prefetch the value during iteration, because the
 * prefetch adds overhead to EXISTS and STORE operations.
 * They are only "penalized" when the perl program is iterating
 * via each(), which seems to be a reasonable tradeoff.
 */

static int apreq_xs_cookie_table_magic_copy(pTHX_ SV *sv, MAGIC *mg, SV *nsv, 
                                            const char *name, int namelen)
{
    /* Prefetch the value whenever the table iterator is > 0 */
    MAGIC *tie_magic = mg_find(nsv, PERL_MAGIC_tiedelem);
    SV *obj = SvRV(tie_magic->mg_obj);
    IV idx = SvIVX(obj);
    const apr_table_t *t = INT2PTR(apr_table_t *, idx);
    const apr_array_header_t *arr = apr_table_elts(t);

    idx = SvCUR(obj);

    if (idx > 0 && idx <= arr->nelts) {
        const apr_table_entry_t *te = (const apr_table_entry_t *)arr->elts;
        apreq_cookie_t *c = apreq_value_to_cookie(te[idx-1].val);
        MAGIC *my_magic = mg_find(obj, PERL_MAGIC_ext);

        SvMAGICAL_off(nsv);
        sv_setsv(nsv, sv_2mortal(apreq_xs_cookie2sv(aTHX_ c, my_magic->mg_ptr,
                                                    my_magic->mg_obj)));
    }

    return 0;
}

static const MGVTBL apreq_xs_cookie_table_magic = {0, 0, 0, 0, 0, 
                                apreq_xs_cookie_table_magic_copy};

#endif

static APR_INLINE
SV *apreq_xs_cookie_table2sv(pTHX_ const apr_table_t *t, const char *class, SV *parent,
                      const char *value_class, I32 vclen)
{
    SV *sv = (SV *)newHV();
    SV *rv = sv_setref_pv(newSV(0), class, (void *)t);
    sv_magic(SvRV(rv), parent, PERL_MAGIC_ext, value_class, vclen);

#if (PERL_VERSION >= 8) /* MAGIC ITERATOR requires 5.8 */

    sv_magic(sv, NULL, PERL_MAGIC_ext, Nullch, -1);
    SvMAGIC(sv)->mg_virtual = (MGVTBL *)&apreq_xs_cookie_table_magic;
    SvMAGIC(sv)->mg_flags |= MGf_COPY;

#endif

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}



static int apreq_xs_cookie_table_keys(void *data, const char *key,
                                      const char *val)
{
#ifdef USE_ITHREADS
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
#endif
    dSP;
    apreq_cookie_t *c = apreq_value_to_cookie(val);
    SV *sv = newSVpvn(key, c->v.nlen);
    if (apreq_cookie_is_tainted(c))
        SvTAINTED_on(sv);

#ifndef USE_ITHREADS
		(void)data;
#endif	
    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}

static int apreq_xs_cookie_table_values(void *data, const char *key,
                                        const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
    dSP;
    apreq_cookie_t *c = apreq_value_to_cookie(val);
    SV *sv = apreq_xs_cookie2sv(aTHX_ c, d->pkg, d->parent);

    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}


/**************************************************/


#if (PERL_VERSION >= 8) /* MAGIC ITERATOR REQUIRES 5.8 */

/* Requires perl 5.8 or better. 
 * A custom MGVTBL with its "copy" slot filled allows
 * us to FETCH a table entry immediately during iteration.
 * For multivalued keys this is essential in order to get
 * the value corresponding to the current key, otherwise
 * values() will always report the first value repeatedly.
 * With this MGVTBL the keys() list always matches up with
 * the values() list, even in the multivalued case.
 * We only prefetch the value during iteration, because the
 * prefetch adds overhead to EXISTS and STORE operations.
 * They are only "penalized" when the perl program is iterating
 * via each(), which seems to be a reasonable tradeoff.
 */

static int apreq_xs_param_table_magic_copy(pTHX_ SV *sv, MAGIC *mg, SV *nsv, 
                                  const char *name, int namelen)
{
    /* Prefetch the value whenever the table iterator is > 0 */
    MAGIC *tie_magic = mg_find(nsv, PERL_MAGIC_tiedelem);
    SV *obj = SvRV(tie_magic->mg_obj);
    IV idx = SvIVX(obj);
    const apr_table_t *t = INT2PTR(apr_table_t *, idx);
    const apr_array_header_t *arr = apr_table_elts(t);

    idx = SvCUR(obj);

    if (idx > 0 && idx <= arr->nelts) {
        const apr_table_entry_t *te = (const apr_table_entry_t *)arr->elts;
        apreq_param_t *p = apreq_value_to_param(te[idx-1].val);
        MAGIC *my_magic = mg_find(obj, PERL_MAGIC_ext);

        SvMAGICAL_off(nsv);
        sv_setsv(nsv, sv_2mortal(apreq_xs_param2sv(aTHX_ p, my_magic->mg_ptr, 
                                                   my_magic->mg_obj)));
    }

    return 0;
}

static const MGVTBL apreq_xs_param_table_magic = {0, 0, 0, 0, 0, 
                                 apreq_xs_param_table_magic_copy};

#endif

static APR_INLINE
SV *apreq_xs_param_table2sv(pTHX_ const apr_table_t *t, const char *class, SV *parent,
                            const char *value_class, I32 vclen)
{
    SV *sv = (SV *)newHV();
    SV *rv = sv_setref_pv(newSV(0), class, (void *)t);
    sv_magic(SvRV(rv), parent, PERL_MAGIC_ext, value_class, vclen);

#if (PERL_VERSION >= 8) /* MAGIC ITERATOR requires 5.8 */

    sv_magic(sv, NULL, PERL_MAGIC_ext, Nullch, -1);
    SvMAGIC(sv)->mg_virtual = (MGVTBL *)&apreq_xs_param_table_magic;
    SvMAGIC(sv)->mg_flags |= MGf_COPY;

#endif

    sv_magic(sv, rv, PERL_MAGIC_tied, Nullch, 0);
    SvREFCNT_dec(rv); /* corrects SvREFCNT_inc(rv) implicit in sv_magic */

    return sv_bless(newRV_noinc(sv), SvSTASH(SvRV(rv)));
}



static int apreq_xs_param_table_keys(void *data, const char *key,
                                     const char *val)
{
#ifdef USE_ITHREADS
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
#endif
    dSP;
    apreq_param_t *p = apreq_value_to_param(val);
    SV *sv = newSVpvn(key, p->v.nlen);

#ifndef USE_ITHREADS
    (void)data;
#endif

    if (apreq_param_is_tainted(p))
        SvTAINTED_on(sv);

    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}

static int apreq_xs_param_table_values(void *data, const char *key,
                                       const char *val)
{
    struct apreq_xs_do_arg *d = (struct apreq_xs_do_arg *)data;
    dTHXa(d->perl);
    dSP;
    apreq_param_t *p = apreq_value_to_param(val);
    SV *sv = apreq_xs_param2sv(aTHX_ p, d->pkg, d->parent);

    XPUSHs(sv_2mortal(sv));
    PUTBACK;
    return 1;
}



#endif /* APREQ_XS_TABLES_H */
