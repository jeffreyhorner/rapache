static apr_pool_t *apreq_xs_cgi_global_pool;

MODULE = APR::Request::CGI   PACKAGE = APR::Request::CGI

BOOT:
    {
        apr_version_t version;
        apr_version(&version);
        if (version.major != APR_MAJOR_VERSION)
            Perl_croak(aTHX_ "Can't load module APR::Request::CGI : "
                             "wrong libapr major version "
                             "(expected %d, saw %d)",
                              APR_MAJOR_VERSION, version.major);
    }
    apr_pool_create(&apreq_xs_cgi_global_pool, NULL);
    apreq_initialize(apreq_xs_cgi_global_pool);
