MODULE = APR::Request::Param      PACKAGE = APR::Request::Param

SV *
value(obj, p1=NULL, p2=NULL)
    APR::Request::Param obj
    SV *p1
    SV *p2
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = apreq_xs_param2sv(aTHX_ obj, NULL, NULL);

  OUTPUT:
    RETVAL

SV *
upload_filename(obj)
    APR::Request::Param obj
  PREINIT:

  CODE:
    if (obj->upload != NULL)
        RETVAL = apreq_xs_param2sv(aTHX_ obj, NULL, NULL);
    else
        RETVAL = &PL_sv_undef;

  OUTPUT:
    RETVAL



BOOT:
    {
        apr_version_t version;
        apr_version(&version);
        if (version.major != APR_MAJOR_VERSION)
            Perl_croak(aTHX_ "Can't load module APR::Request::Param : "
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
        get_sv( "APR::Request::Param::()", TRUE ),
        &PL_sv_yes
    );
    newXS("APR::Request::Param::()", XS_APR__Request__Param_nil, file);
    newXS("APR::Request::Param::(\"\"", XS_APR__Request__Param_value, file);


MODULE = APR::Request::Param   PACKAGE = APR::Request::Param

SV *
name(obj)
    APR::Request::Param obj

  CODE:
    RETVAL = newSVpvn(obj->v.name, obj->v.nlen);
    if (apreq_param_is_tainted(obj))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL


IV
is_tainted(obj, val=NULL)
    APR::Request::Param obj
    SV *val
  PREINIT:
    /*nada*/

  CODE:
    RETVAL = apreq_param_is_tainted(obj);

    if (items == 2) {
        if (SvTRUE(val))
           apreq_param_tainted_on(obj);
        else
           apreq_param_tainted_off(obj);
    }

  OUTPUT:
    RETVAL

IV
charset(obj, val=NULL)
    APR::Request::Param obj
    SV *val
  PREINIT:
    /*nada*/

  CODE:
    if (items == 2)
        RETVAL = apreq_param_charset_set(obj, SvIV(val));
    else
        RETVAL = apreq_param_charset_get(obj);

  OUTPUT:
    RETVAL

APR::Request::Param
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
    RETVAL = apreq_param_make(pool, n, nlen, v, vlen);
    if (SvTAINTED(name) || SvTAINTED(val))
        apreq_param_tainted_on(RETVAL);

  OUTPUT:
    RETVAL


MODULE = APR::Request::Param PACKAGE = APR::Request::Param::Table

SV *
param_class(t, newclass=NULL)
    APR::Request::Param::Table t
    char *newclass

  PREINIT:
    SV *obj = apreq_xs_sv2object(aTHX_ ST(0), PARAM_TABLE_CLASS, 't');
    MAGIC *mg = mg_find(obj, PERL_MAGIC_ext);
    char *curclass = mg->mg_ptr;

  CODE:
    RETVAL = (curclass == NULL) ? &PL_sv_undef : newSVpv(curclass, 0);

    if (newclass != NULL) {
        if (!sv_derived_from(ST(1), PARAM_CLASS))
            Perl_croak(aTHX_ "Usage: "
                              PARAM_TABLE_CLASS "::param_class($table, $class): "
                             "class %s is not derived from " PARAM_CLASS, newclass);
        mg->mg_ptr = savepv(newclass);
        mg->mg_len = strlen(newclass);

        if (curclass != NULL)
            Safefree(curclass);
    }

  OUTPUT:
    RETVAL


MODULE = APR::Request::Param PACKAGE = APR::Request::Param

SV *
upload_link(param, path)
    APR::Request::Param param
    const char *path
  PREINIT:
    apr_file_t *f;
    const char *fname;
    apr_status_t s;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_link($file): param has no upload brigade");
    f = apreq_brigade_spoolfile(param->upload);
    if (f == NULL) {
        apr_off_t len;
        s = apr_file_open(&f, path, APR_CREATE | APR_EXCL | APR_WRITE |
                          APR_READ | APR_BINARY,
                          APR_OS_DEFAULT,
                          param->upload->p);
        if (s == APR_SUCCESS) {
            s = apreq_brigade_fwrite(f, &len, param->upload);
            if (s == APR_SUCCESS)
                XSRETURN_YES;
        }
    }
    else {
        s = apr_file_name_get(&fname, f);
        if (s != APR_SUCCESS)
            Perl_croak(aTHX_ "$param->upload_link($file): can't get spoolfile name");
        if (PerlLIO_link(fname, path) >= 0)
            XSRETURN_YES;
        else {
            s = apr_file_copy(fname, path, APR_OS_DEFAULT, param->upload->p);
            if (s == APR_SUCCESS)
                XSRETURN_YES;
        }
    }
    RETVAL = &PL_sv_undef;

  OUTPUT:
    RETVAL

apr_size_t
upload_slurp(param, buffer)
    APR::Request::Param param
    SV *buffer
  PREINIT:
    apr_off_t len;
    apr_status_t s;
    char *data;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_slurp($data): param has no upload brigade");

    s = apr_brigade_length(param->upload, 0, &len);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_slurp($data): can't get upload length");

    RETVAL = len;
    SvUPGRADE(buffer, SVt_PV);
    data = SvGROW(buffer, RETVAL + 1);
    data[RETVAL] = 0;
    SvCUR_set(buffer, RETVAL);
    SvPOK_only(buffer);
    s = apr_brigade_flatten(param->upload, data, &RETVAL);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_slurp($data): can't flatten upload");

    if (apreq_param_is_tainted(param))
        SvTAINTED_on(buffer);

    SvSETMAGIC(buffer);

  OUTPUT:
    RETVAL

UV
upload_size(param)
    APR::Request::Param param
  PREINIT:
    apr_off_t len;
    apr_status_t s;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_size(): param has no upload brigade");

    s = apr_brigade_length(param->upload, 0, &len);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_size(): can't get upload length");

    RETVAL = len;    

  OUTPUT:
    RETVAL

SV *
upload_type(param)
    APR::Request::Param param
  PREINIT:
    const char *ct, *sc;
    STRLEN len;
  CODE:
    if (param->info == NULL)
        Perl_croak(aTHX_ "$param->upload_type(): param has no info table");

    ct = apr_table_get(param->info, "Content-Type");
    if (ct == NULL)
        Perl_croak(aTHX_ "$param->upload_type: can't find Content-Type header");
    
    if ((sc = strchr(ct, ';')))
        len = sc - ct;
    else
        len = strlen(ct);

    RETVAL = newSVpvn(ct, len);    
    if (apreq_param_is_tainted(param))
        SvTAINTED_on(RETVAL);

  OUTPUT:
    RETVAL


const char *
upload_tempname(param, req=apreq_xs_sv2handle(aTHX_ ST(0)))
    APR::Request::Param param
    APR::Request req

  PREINIT:
    apr_file_t *f;
    apr_status_t s;

  CODE:
    if (param->upload == NULL)
        Perl_croak(aTHX_ "$param->upload_tempname($req): param has no upload brigade");
    f = apreq_brigade_spoolfile(param->upload);
    if (f == NULL) {
        const char *path;
        s = apreq_temp_dir_get(req, &path);
        if (s != APR_SUCCESS)
            Perl_croak(aTHX_ "$param->upload_tempname($req): can't get temp_dir");
        s = apreq_brigade_concat(param->upload->p, path, 0, 
                                 param->upload, param->upload);
        if (s != APR_SUCCESS)
            Perl_croak(aTHX_ "$param->upload_tempname($req): can't make spool bucket");
        f = apreq_brigade_spoolfile(param->upload);
    }
    s = apr_file_name_get(&RETVAL, f);
    if (s != APR_SUCCESS)
        Perl_croak(aTHX_ "$param->upload_link($file): can't get spool file name");

  OUTPUT:
    RETVAL



