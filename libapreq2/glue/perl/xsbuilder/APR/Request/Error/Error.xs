MODULE = APR::Request::Error       PACKAGE = APR::Request::Error

SV *strerror(s)
    apr_status_t s
  CODE:
    RETVAL = apreq_xs_error2sv(aTHX_ s);
  OUTPUT:
    RETVAL

SV *as_string(hv, p1=NULL, p2=NULL)
    APR::Request::Error hv
    SV *p1
    SV *p2
  PREINIT:
    SV **svp;

  CODE:
    svp = hv_fetch(hv, "rc", 2, FALSE);
    if (svp == NULL)
        RETVAL = &PL_sv_undef;
    else
        RETVAL = apreq_xs_error2sv(aTHX_ SvIVX(*svp));

  OUTPUT:
    RETVAL

BOOT:
    {
        apr_version_t version;
        apr_version(&version);
        if (version.major != APR_MAJOR_VERSION)
            Perl_croak(aTHX_ "Can't load module APR::Request::Error : "
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
        get_sv( "APR::Request::Error::()", TRUE ),
        &PL_sv_undef
    );
    newXS("APR::Request::Error::()", XS_APR__Request__Error_nil, file);
    newXS("APR::Request::Error::(\"\"", XS_APR__Request__Error_as_string, file);

    newCONSTSUB(PL_defstash, "APR::Request::Error::GENERAL",
                apreq_xs_error2sv(aTHX_ APREQ_ERROR_GENERAL));
    newCONSTSUB(PL_defstash, "APR::Request::Error::TAINTED",
                apreq_xs_error2sv(aTHX_ APREQ_ERROR_TAINTED));
