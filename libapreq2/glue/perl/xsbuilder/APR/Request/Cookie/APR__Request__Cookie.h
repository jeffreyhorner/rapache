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

static int apreq_xs_cookie_table_do_sub(void *data, const char *key,
                                       const char *val)
{
    struct apreq_xs_do_arg *d = data;
    apreq_cookie_t *c = apreq_value_to_cookie(val);
    dTHXa(d->perl);
    dSP;
    SV *sv = apreq_xs_cookie2sv(aTHX_ c, d->pkg, d->parent);
    int rv;

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    EXTEND(SP,2);

    PUSHs(sv_2mortal(newSVpvn(c->v.name, c->v.nlen)));
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

static XS(apreq_xs_cookie_table_do)
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

    t_obj = apreq_xs_sv2object(aTHX_ sv, COOKIE_TABLE_CLASS, 't');
    iv = SvIVX(t_obj);
    t = INT2PTR(const apr_table_t *, iv);
    mg = mg_find(t_obj, PERL_MAGIC_ext);
    d.parent = mg->mg_obj;
    d.pkg = mg->mg_ptr;
    d.sub = ST(1);

    if (items == 2) {
        rv = apr_table_do(apreq_xs_cookie_table_do_sub, &d, t, NULL);
        XSRETURN_IV(rv);
    }

    for (i = 2; i < items; ++i) {
        const char *key = SvPV_nolen(ST(i));
        rv = apr_table_do(apreq_xs_cookie_table_do_sub, &d, t, key, NULL);
        if (rv == 0)
            break;
    }
    XSRETURN_IV(rv);
}

static XS(apreq_xs_cookie_table_FETCH)
{
    dXSARGS;
    const apr_table_t *t;
    const char *cookie_class;
    SV *sv, *obj, *parent;
    IV iv;
    MAGIC *mg;

    if (items != 2 || !SvROK(ST(0))
        || !sv_derived_from(ST(0), COOKIE_TABLE_CLASS))
        Perl_croak(aTHX_ "Usage: " COOKIE_TABLE_CLASS "::FETCH($table, $key)");

    sv = ST(0);

    obj = apreq_xs_sv2object(aTHX_ sv, COOKIE_TABLE_CLASS, 't');
    iv = SvIVX(obj);
    t = INT2PTR(const apr_table_t *, iv);

    mg = mg_find(obj, PERL_MAGIC_ext);
    cookie_class = mg->mg_ptr;
    parent = mg->mg_obj;

    if (GIMME_V == G_SCALAR) {
        IV idx;
        const char *key, *val;
        const apr_array_header_t *arr;
        apr_table_entry_t *te;
        key = SvPV_nolen(ST(1));

        idx = SvCUR(obj);
        arr = apr_table_elts(t);
        te  = (apr_table_entry_t *)arr->elts;

        if (idx > 0 && idx <= arr->nelts
            && !strcasecmp(key, te[idx-1].key))
            val = te[idx-1].val;
        else
            val = apr_table_get(t, key);

        if (val != NULL) {
            apreq_cookie_t *c = apreq_value_to_cookie(val);
            ST(0) = apreq_xs_cookie2sv(aTHX_ c, cookie_class, parent);
            sv_2mortal(ST(0));
            XSRETURN(1);
        }
        else {
            XSRETURN_UNDEF;
        }
    }
    else if (GIMME_V == G_ARRAY) {
        struct apreq_xs_do_arg d = {NULL, NULL, NULL, aTHX};
        d.pkg = cookie_class;
        d.parent = parent;
        XSprePUSH;
        PUTBACK;
        apr_table_do(apreq_xs_cookie_table_values, &d, t, SvPV_nolen(ST(1)), NULL);
    }
    else
        XSRETURN(0);
}

static XS(apreq_xs_cookie_table_NEXTKEY)
{
    dXSARGS;
    SV *sv, *obj;
    IV iv, idx;
    const apr_table_t *t;
    const apr_array_header_t *arr;
    apr_table_entry_t *te;

    if (!SvROK(ST(0)))
        Perl_croak(aTHX_ "Usage: $table->NEXTKEY($prev)");

    sv  = ST(0);
    obj = apreq_xs_sv2object(aTHX_ sv, COOKIE_TABLE_CLASS, 't');

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


static XS(XS_APR__Request__Cookie_nil)
{
    dXSARGS;
    (void)items;
    XSRETURN_EMPTY;
}
