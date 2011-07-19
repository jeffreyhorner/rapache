MODULE = APR::Request::Cookie      PACKAGE = APR::Request::Cookie

SV *
value(obj, p1=NULL, p2=NULL)
    APR::Request::Cookie obj
    SV *p1
    SV *p2
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = newSVpvn(obj->v.data, obj->v.dlen);
    if (apreq_cookie_is_tainted(obj))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL



BOOT:
    {
        apr_version_t version;
        apr_version(&version);
        if (version.major != APR_MAJOR_VERSION)
            Perl_croak(aTHX_ "Can't load module APR::Request::Cookie : "
                             "wrong libapr major version "
                             "(expected %d, saw %d)",
                              APR_MAJOR_VERSION, version.major);
    }

    /* register the overloading (type 'A') magic */
    PL_amagic_generation++;
    /* The magic for overload gets a GV* via gv_fetchmeth as */
    /* mentioned above, and looks in the SV* slot of it for */
    /* the "fallback" status. */
    sv_setsv(
        get_sv( "APR::Request::Cookie::()", TRUE ),
        &PL_sv_yes
    );
    newXS("APR::Request::Cookie::()", XS_APR__Request__Cookie_nil, file);
    newXS("APR::Request::Cookie::(\"\"", XS_APR__Request__Cookie_value, file);


MODULE = APR::Request::Cookie   PACKAGE = APR::Request::Cookie

SV *
name(obj)
    APR::Request::Cookie obj

  CODE:
    RETVAL = newSVpvn(obj->v.name, obj->v.nlen);
    if (apreq_cookie_is_tainted(obj))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL

UV
secure(obj, val=NULL)
    APR::Request::Cookie obj
    SV *val

  CODE:
    RETVAL = apreq_cookie_is_secure(obj);
    if (items == 2) {
        if (SvTRUE(val))
            apreq_cookie_secure_on(obj);
        else
            apreq_cookie_secure_off(obj);
    }

  OUTPUT:
    RETVAL

UV
version(obj, val=0)
    APR::Request::Cookie obj
    UV val

  CODE:
    RETVAL = apreq_cookie_version(obj);
    if (items == 2)
        apreq_cookie_version_set(obj, val);
 
  OUTPUT:
    RETVAL

IV
is_tainted(obj, val=NULL)
    APR::Request::Cookie obj
    SV *val
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = apreq_cookie_is_tainted(obj);

    if (items == 2) {
        if (SvTRUE(val))
           apreq_cookie_tainted_on(obj);
        else
           apreq_cookie_tainted_off(obj);
    }

  OUTPUT:
    RETVAL


SV*
bind_handle(cookie, req)
    SV *cookie
    SV *req
  PREINIT:
    MAGIC *mg;
    SV *obj;
  CODE:
    obj = apreq_xs_sv2object(aTHX_ cookie, COOKIE_CLASS, 'c');
    mg = mg_find(obj, PERL_MAGIC_ext);
    req = apreq_xs_sv2object(aTHX_ req, HANDLE_CLASS, 'r');
    RETVAL = newRV_noinc(mg->mg_obj);
    SvREFCNT_inc(req);
    mg->mg_obj = req;

  OUTPUT:
    RETVAL


APR::Request::Cookie
make(class, pool, name, val)
    apreq_xs_subclass_t class
    APR::Pool pool
    SV *name
    SV *val
  PREINIT:
    STRLEN nlen, vlen;
    const char *n, *v;
    SV *parent = SvRV(ST(1));

  CODE:
    n = SvPV(name, nlen);
    v = SvPV(val, vlen);
    RETVAL = apreq_cookie_make(pool, n, nlen, v, vlen);
    if (SvTAINTED(name) || SvTAINTED(val))
        apreq_cookie_tainted_on(RETVAL);

  OUTPUT:
    RETVAL

SV *
as_string(c)
    APR::Request::Cookie c
  PREINIT:
    char rv[APREQ_COOKIE_MAX_LENGTH];
    STRLEN len;

  CODE:
    len = apreq_cookie_serialize(c, rv, sizeof rv);
    RETVAL = newSVpvn(rv, len);
    if (apreq_cookie_is_tainted(c))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL

MODULE = APR::Request::Cookie PACKAGE = APR::Request::Cookie::Table

SV *
cookie_class(t, newclass=NULL)
    APR::Request::Cookie::Table t
    char *newclass

  PREINIT:
    SV *obj = apreq_xs_sv2object(aTHX_ ST(0), COOKIE_TABLE_CLASS, 't');
    MAGIC *mg = mg_find(obj, PERL_MAGIC_ext);
    char *curclass = mg->mg_ptr;

  CODE:
    RETVAL = (curclass == NULL) ? &PL_sv_undef : newSVpv(curclass, 0);

    if (newclass != NULL) {
        if (!sv_derived_from(ST(1), COOKIE_CLASS))
            Perl_croak(aTHX_ "Usage: " 
                             COOKIE_TABLE_CLASS "::cookie_class($table, $class): "
                             "class %s is not derived from " COOKIE_CLASS, newclass);
        mg->mg_ptr = savepv(newclass);
        mg->mg_len = strlen(newclass);

        if (curclass != NULL)
            Safefree(curclass);
    }

  OUTPUT:
    RETVAL

