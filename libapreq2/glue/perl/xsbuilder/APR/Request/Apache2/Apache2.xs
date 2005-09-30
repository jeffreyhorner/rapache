MODULE = APR::Request::Apache2    PACKAGE = APR::Request::Apache2

BOOT:
    {
        apr_version_t version;
        apr_version(&version);
        if (version.major != APR_MAJOR_VERSION)
            Perl_croak(aTHX_ "Can't load module APR::Request::Apache2 : "
                             "wrong libapr major version "
                             "(expected %d, saw %d)",
                              APR_MAJOR_VERSION, version.major);
    }
