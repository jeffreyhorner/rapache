/*
 * Definitions to satisfy R CMD check
 */

AP_DECLARE(apr_status_t) ap_filter_flush( apr_bucket_brigade *bb, void *ctx){ return APR_SUCCESS;}
apr_bucket_brigade *MR_bbin;
apr_bucket_brigade *MR_bbout;
SEXP (*RA_new_request_rec)(request_rec *r);
SEXP ra_new_request_rec(request_rec *r);
AP_DECLARE(void) ap_log_rerror(const char *file, int line, int level, 
                               apr_status_t status, const request_rec *r, 
                               const char *fmt, ...){ return; }
AP_DECLARE(apr_status_t) ap_get_brigade(ap_filter_t *filter, 
                                        apr_bucket_brigade *bucket, 
                                        ap_input_mode_t mode,
                                        apr_read_type_e block, 
                                        apr_off_t readbytes){ return APR_SUCCESS;}
AP_DECLARE(void) ap_add_cgi_vars(request_rec *r){ return; }
AP_DECLARE(void) ap_add_common_vars(request_rec *r){ return; }
AP_DECLARE(void) ap_set_content_type(request_rec *r, const char *ct){ return; }
AP_DECLARE(const char *) ap_method_name_of(apr_pool_t *p, int methnum){ return NULL;}
AP_DECLARE(void) ap_allow_methods(request_rec *r, int reset, ...){ return; }
