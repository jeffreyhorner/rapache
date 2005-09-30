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


static int apreq_xs_param_table_do_sub(void *data, const char *key,
                                       const char *val)
{
    struct apreq_xs_do_arg *d = data;
    apreq_param_t *p = apreq_value_to_param(val);
    dTHXa(d->perl);
    dSP;
    SV *sv = apreq_xs_param2sv(aTHX_ p, d->pkg, d->parent);
    int rv;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    EXTEND(SP,2);

    PUSHs(sv_2mortal(newSVpvn(p->v.name, p->v.nlen)));
    PUSHs(sv_2mortal(sv));

    PUTBACK;
    rv = call_sv(d->sub, G_SCALAR);
    SPAGAIN;
    rv = (1 == rv) ? POPi : 1;
    PUTBACK;
    FREETMPS;
    LEAVE;

    return rv;
}

static XS(apreq_xs_param_table_do)
{
    dXSARGS;
    struct apreq_xs_do_arg d = { NULL, NULL, NULL, aTHX };
    const apr_table_t *t;
    int i, rv = 1;
    SV *sv, *t_obj;
    IV iv;
    MAGIC *mg;

    if (items < 2 || !SvROK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: $object->do(\\&callback, @keys)");
    sv = ST(0);

    t_obj = apreq_xs_sv2object(aTHX_ sv, PARAM_TABLE_CLASS, 't');
    iv = SvIVX(t_obj);
    t = INT2PTR(const apr_table_t *, iv);
    mg = mg_find(t_obj, PERL_MAGIC_ext);
    d.parent = mg->mg_obj;
    d.pkg = mg->mg_ptr;
    d.sub = ST(1);

    if (items == 2) {
        rv = apr_table_do(apreq_xs_param_table_do_sub, &d, t, NULL);
        XSRETURN_IV(rv);
    }

    for (i = 2; i < items; ++i) {
        const char *key = SvPV_nolen(ST(i));
        rv = apr_table_do(apreq_xs_param_table_do_sub, &d, t, key, NULL);
        if (rv == 0)
            break;
    }
    XSRETURN_IV(rv);
}

static XS(apreq_xs_param_table_FETCH)
{
    dXSARGS;
    const apr_table_t *t;
    const char *param_class;
    SV *sv, *t_obj, *parent;
    IV iv;
    MAGIC *mg;

    if (items != 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), PARAM_TABLE_CLASS))
        Perl_croak(aTHX_ "Usage: " PARAM_TABLE_CLASS "::FETCH($table, $key)");

    sv = ST(0);

    t_obj = apreq_xs_sv2object(aTHX_ sv, PARAM_TABLE_CLASS, 't');
    iv = SvIVX(t_obj);
    t = INT2PTR(const apr_table_t *, iv);

    mg = mg_find(t_obj, PERL_MAGIC_ext);
    param_class = mg->mg_ptr;
    parent = mg->mg_obj;


    if (GIMME_V == G_SCALAR) {
        IV idx;
        const char *key, *val;
        const apr_array_header_t *arr;
        apr_table_entry_t *te;
        key = SvPV_nolen(ST(1));

        idx = SvCUR(t_obj);
        arr = apr_table_elts(t);
        te  = (apr_table_entry_t *)arr->elts;

        if (idx > 0 && idx <= arr->nelts
            && !strcasecmp(key, te[idx-1].key))
            val = te[idx-1].val;
        else
            val = apr_table_get(t, key);

        if (val != NULL) {
            apreq_param_t *p = apreq_value_to_param(val);
            ST(0) = apreq_xs_param2sv(aTHX_ p, param_class, parent);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            XSRETURN_UNDEF;
        }
    }
    else if (GIMME_V == G_ARRAY) {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        d.pkg = param_class;
        d.parent = parent;
        XSprePUSH;
        PUTBACK;
        apr_table_do(apreq_xs_param_table_values, &d, t, SvPV_nolen(ST(1)), NULL);
    }
    else
        XSRETURN(0);
}

static XS(apreq_xs_param_table_NEXTKEY)
{
    dXSARGS;
    SV *sv, *obj;
    IV iv, idx;
    const apr_table_t *t;
    const apr_array_header_t *arr;
    apr_table_entry_t *te;

    if (!SvROK(ST(0)) || !sv_derived_from(ST(0), PARAM_TABLE_CLASS))
        Perl_croak(aTHX_ "Usage: " PARAM_TABLE_CLASS "::NEXTKEY($table, $key)");

    sv  = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, PARAM_TABLE_CLASS,'t');

    iv = SvIVX(obj);
    t = INT2PTR(const apr_table_t *, iv);
    arr = apr_table_elts(t);
    te  = (apr_table_entry_t *)arr->elts;

    if (items == 1)
        SvCUR(obj) = 0;

    if (SvCUR(obj) >= arr->nelts) {
        SvCUR(obj) = 0;
        XSRETURN_UNDEF;
    }
    idx = SvCUR(obj)++;
    sv = newSVpv(te[idx].key, 0);
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

 
static XS(XS_APR__Request__Param_nil)
{
    dXSARGS;
    (void)items;
    XSRETURN_EMPTY;
}


APR_INLINE
static SV *apreq_xs_find_bb_obj(pTHX_ SV *in)
{
    while (in && SvROK(in)) {
        SV *sv = SvRV(in);
        switch (SvTYPE(sv)) {            
            MAGIC *mg;
        case SVt_PVIO:
            if (SvMAGICAL(sv) && (mg = mg_find(sv, PERL_MAGIC_tiedscalar))) {
               in = mg->mg_obj;
               break;
            }
            Perl_croak(aTHX_ "panic: cannot find tied scalar in pvio magic");
        case SVt_PVMG:
            if (SvOBJECT(sv) && SvIOKp(sv))
                return sv;
        default:
             Perl_croak(aTHX_ "panic: unsupported SV type: %d", SvTYPE(sv));
       }
    }
    return in;
}

/* XXX these Apache::Upload::Brigade funcs need a makeover as vanilla XS. */

static XS(apreq_xs_brigade_copy)
{
    dXSARGS;
    apr_bucket_brigade *bb, *bb_copy;
    char *class;
    SV *sv, *obj;
    IV iv;

    if (items != 2 || !SvPOK(ST(0)) || !SvROK(ST(1)))
        Perl_croak(aTHX_ "Usage: APR::Request::Brigade->new($bb)");

    class = SvPV_nolen(ST(0));
    obj = apreq_xs_find_bb_obj(aTHX_ ST(1));
    iv = SvIVX(obj);
    bb = INT2PTR(apr_bucket_brigade *, iv);
    bb_copy = apr_brigade_create(bb->p, bb->bucket_alloc);
    apreq_brigade_copy(bb_copy, bb);

    sv = sv_setref_pv(newSV(0), class, bb_copy);
    if (SvTAINTED(obj))
        SvTAINTED_on(SvRV(sv));
    ST(0) = sv_2mortal(sv);
    XSRETURN(1);
}

static XS(apreq_xs_brigade_read)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e, *end;
    IV want = -1, offset = 0;
    SV *sv, *obj;
    apr_status_t s;
    char *buf;

    switch (items) {
    case 4:
        offset = SvIV(ST(3));
    case 3:
        want = SvIV(ST(2));
    case 2:
        sv = ST(1);
        SvUPGRADE(sv, SVt_PV);
        if (SvROK(ST(0))) {
            IV iv;
            obj = apreq_xs_find_bb_obj(aTHX_ ST(0));
            iv = SvIVX(obj);
            bb = INT2PTR(apr_bucket_brigade *, iv);
            break;
        }
    default:
        Perl_croak(aTHX_ "Usage: $bb->READ($buf,$len,$off)");
    }

    if (want == 0) {
        SvCUR_set(sv, offset);
        XSRETURN_IV(0);
    }

    if (APR_BRIGADE_EMPTY(bb)) {
        SvCUR_set(sv, offset);
        XSRETURN_UNDEF;
    }

    if (want == -1) {
        const char *data;
        apr_size_t dlen;
        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "APR::Request::Brigade::READ", 
                           "APR::Error");
        want = dlen;
        end = APR_BUCKET_NEXT(e);
    }
    else {
        switch (s = apr_brigade_partition(bb, (apr_off_t)want, &end)) {
            apr_off_t len;

        case APR_INCOMPLETE:
            s = apr_brigade_length(bb, 1, &len);
            if (s != APR_SUCCESS)
                apreq_xs_croak(aTHX_ newHV(), s, 
                               "APR::Request::Brigade::READ",
                               "APR::Error");
            want = len;

        case APR_SUCCESS:
            break;

        default:
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "APR::Request::Brigade::READ",
                           "APR::Error");
        }
    }

    SvGROW(sv, want + offset + 1);
    buf = SvPVX(sv) + offset;
    SvCUR_set(sv, want + offset);
    if (SvTAINTED(obj))
        SvTAINTED_on(sv);

    while ((e = APR_BRIGADE_FIRST(bb)) != end) {
        const char *data;
        apr_size_t dlen;
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "APR::Request::Brigade::READ", "APR::Error");
        memcpy(buf, data, dlen);
        buf += dlen;
        apr_bucket_delete(e);
    }

    *buf = 0;
    SvPOK_only(sv);
    SvSETMAGIC(sv);
    XSRETURN_IV(want);
}

static XS(apreq_xs_brigade_readline)
{
    dXSARGS;
    apr_bucket_brigade *bb;
    apr_bucket *e;
    SV *sv, *obj;
    IV iv;
    apr_status_t s;
    unsigned tainted;

    if (items != 1 || !SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $bb->READLINE");

    obj = apreq_xs_find_bb_obj(aTHX_ ST(0));
    iv = SvIVX(obj);
    bb = INT2PTR(apr_bucket_brigade *, iv);

    if (APR_BRIGADE_EMPTY(bb))
        XSRETURN(0);

    tainted = SvTAINTED(obj);

    XSprePUSH;

    sv = sv_2mortal(newSVpvn("",0));
    if (tainted)
        SvTAINTED_on(sv);
        
    XPUSHs(sv);

    while (!APR_BRIGADE_EMPTY(bb)) {
        const char *data;
        apr_size_t dlen;
        const char *eol;

        e = APR_BRIGADE_FIRST(bb);
        s = apr_bucket_read(e, &data, &dlen, APR_BLOCK_READ);
        if (s != APR_SUCCESS)
            apreq_xs_croak(aTHX_ newHV(), s, 
                           "APR::Request::Brigade::READLINE",
                           "APR::Error");

        eol = memchr(data, '\012', dlen); /* look for LF (linefeed) */

        if (eol != NULL) {
            if (eol < data + dlen - 1) {
                dlen = eol - data + 1;
                apr_bucket_split(e, dlen);
            }

            sv_catpvn(sv, data, dlen);
            apr_bucket_delete(e);

            if (GIMME_V != G_ARRAY || APR_BRIGADE_EMPTY(bb))
                break;

            sv = sv_2mortal(newSVpvn("",0));
            if (tainted)
                SvTAINTED_on(sv);
            XPUSHs(sv);
        }
        else {
            sv_catpvn(sv, data, dlen);
            apr_bucket_delete(e);
        }
    }

    PUTBACK;
}

