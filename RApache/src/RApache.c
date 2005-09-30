/*
**  Copyright 2005  The Apache Software Foundation
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

/* offsetof() stuff */
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#include <time.h>

#ifndef offsetof
#define offsetof(type, member) ( (int) & ((type*)0) -> member )
#endif

#include <R.h>
#include <Rdefines.h>
#include <Rdevices.h>
#include <R_ext/Parse.h>
#include <R_ext/Rdynload.h>

#include "httpd.h"
#include "http_protocol.h"
#include "http_config.h"
#include "http_log.h"
#include "http_request.h"
#include "util_filter.h"
#include "util_script.h"
#include "apr_file_info.h"
#include "apr_file_io.h"
#include "apr_strings.h"

#include "apreq.h"
#include "apreq_cookie.h"
#include "apreq_parser.h"
#include "apreq_param.h"
#include "apreq_util.h"

#ifdef HAVE_GDLIB
#include "gd.h"
#include "gd_io.h"

typedef struct {
	gdIOCtx ioctx;
	request_rec *r;
} ra_gd_ioctx;

static int ra_gd_getC(gdIOCtxPtr);
static int ra_gd_getBuf(gdIOCtxPtr, void *, int);
static void ra_gd_putC(gdIOCtxPtr, int);
static int ra_gd_putBuf(gdIOCtxPtr, const void *, int);
static int ra_gd_seek(gdIOCtxPtr, const int);
static long ra_gd_tell(gdIOCtxPtr);
static void ra_gd_free(gdIOCtxPtr);

#endif /* HAVE_GDLIB */

typedef struct {
	char *member;
	SEXP (*fun)(void *,int);
	int offset;
} ra_struct_def_t;

/* mod_R request record */
typedef struct {
	request_rec *r;
	int parsed;
	int read_start;
	apr_table_t *args;
	apr_table_t *post;
	apr_table_t *cookies;
	SEXP uploads;
} ra_request_rec;


/* Functions used in .Call() */
SEXP RA_request_rec_idx(SEXP r,SEXP mbr);
SEXP RA_request_rec_idxc(SEXP r,SEXP mbr);
SEXP RA_request_rec_names(SEXP r);
SEXP RA_request_rec_length(SEXP r);
SEXP RA_apr_table_idx(SEXP t,SEXP mbr);
SEXP RA_apr_table_idxi(SEXP t,SEXP mbr);
SEXP RA_apr_table_idxc(SEXP t,SEXP mbr);
SEXP RA_apr_table_names(SEXP t);
SEXP RA_apr_table_length(SEXP t);
SEXP RA_apr_table_set(SEXP t, SEXP h, SEXP v);
SEXP RA_read(SEXP r, SEXP slen);
SEXP RA_readline(SEXP r, SEXP slen);
SEXP RA_write(SEXP r, SEXP str, SEXP flush);
SEXP RA_get_args(SEXP r);
SEXP RA_get_post(SEXP r);
SEXP RA_get_cookies(SEXP r);
SEXP RA_get_uploads(SEXP r);
SEXP RA_ap_add_common_vars(SEXP r);
SEXP RA_log_error(SEXP r, SEXP msg, SEXP level);
SEXP RA_allow_methods(SEXP r, SEXP replace, SEXP methods);
SEXP RA_gdlib_ioctx(SEXP r);
SEXP RA_ap_set_content_type(SEXP r, SEXP type);

/* Internal Functions */
static void *ra_unmarshall_pointer(SEXP exp, SEXP type);
static SEXP ra_make_apr_table_sexp(apr_table_t *t);
static SEXP ra_getstrmbr(void *r, int offset);
static SEXP ra_getintmbr(void *r, int offset);
static SEXP ra_getlglmbr(void *r, int offset);
/* static SEXP ra_not_implemented(void *, int); */
/* static SEXP ra_getrealmbr(void *r, int offset); */
static SEXP ra_gettimembr(void *r, int offset);
static SEXP ra_gettablembr(void *r, int offset);
static SEXP ra_getoffmbr(void *r, int offset);
static SEXP ra_logical(int);

/*
 * Types and class declarations for external pointers.
 */

SEXP RA_REQUEST_REC_type;
SEXP RA_REQUEST_REC_class;

SEXP RA_APR_TABLE_type;
SEXP RA_APR_TABLE_class;

#ifndef RCHECK
/*
 * Brigades for reading and writing
 */
extern apr_bucket_brigade *MR_bbin;
extern apr_bucket_brigade *MR_bbout;

/* Functions called by mod_R */
extern SEXP (*RA_new_request_rec)(request_rec *r);
SEXP ra_new_request_rec(request_rec *r);
#else
#include "rcheck.h"
#endif

/*
 * Pool used for libapreq
 */
static apr_pool_t *RApache_pool;
/*
 * request_rec structure definition
 */
#define INTMBR(name)  {#name, &ra_getintmbr, offsetof(request_rec,name) }
#define LGLMBR(name)  {#name, &ra_getlglmbr, offsetof(request_rec,name) }
#define STRMBR(name)  {#name, &ra_getstrmbr, offsetof(request_rec,name) }
#define OFFMBR(name) {#name, &ra_getoffmbr, offsetof(request_rec,name) }
#define TABMBR(name) {#name, &ra_gettablembr, offsetof(request_rec,name) }
static ra_struct_def_t request_rec_def[] = 
{
	TABMBR(headers_in),
	TABMBR(headers_out),
	TABMBR(err_headers_out),
	/* TABMBR(subprocess_env), */
	/* TABMBR(notes),  */
	/* LGLMBR(read_chunked), */
	/* INTMBR(expecting_100), */
	/* LGLMBR(chunked), */
	INTMBR(proto_num),
	STRMBR(protocol),
	STRMBR(unparsed_uri),
	STRMBR(uri),
	STRMBR(canonical_filename),
	STRMBR(path_info),
	STRMBR(args),
	STRMBR(content_type),
	STRMBR(handler),
	STRMBR(content_encoding),
	STRMBR(range),
	STRMBR(hostname),
	STRMBR(user),
	LGLMBR(header_only),
	LGLMBR(no_cache),
	LGLMBR(no_local_copy),
	LGLMBR(assbackwards),
	INTMBR(status),
	INTMBR(method_number),
	/* LGLMBR(used_path_info),*/
	LGLMBR(eos_sent),
	STRMBR(the_request),
	STRMBR(method),
	STRMBR(status_line),
	/* OFFMBR(sent_bodyct), */
	OFFMBR(bytes_sent),
	OFFMBR(clength),
	OFFMBR(remaining),
	OFFMBR(read_length),

	{"request_time",&ra_gettimembr,offsetof(request_rec,request_time)},
	{"mtime",&ra_gettimembr,offsetof(request_rec,mtime)},

	/* The rest are not implemented yet */
	/*
	   STRMBR(vlist_validator),
	   INTMBR(read_body),
	   INTMBR(proxyreq),
	   {"server",&ra_getsrvmbr,offsetof(request_rec,server)},
	   NULLMBR(allowed),
	   NULLMBR(allowed_xmethods),
	   NULLMBR(allowed_methods),
	   NULLMBR(main),
	   NULLMBR(next),
	   NULLMBR(prev),
	   NULLMBR(pool),
	   NULLMBR(connection),
	   NULLMBR(content_languages),
	   NULLMBR(finfo),
	   NULLMBR(parsed_uri),
	   NULLMBR(per_dir_config),
	   NULLMBR(request_config),
	   NULLMBR(htaccess),
	   NULLMBR(output_filters),
	   NULLMBR(input_filters),
	   NULLMBR(proto_output_filters),
	   NULLMBR(proto_input_filters), */
	{NULL,NULL,0}
};


SEXP ra_new_request_rec(request_rec *r){

	ra_request_rec *req;
	SEXP sreq;

	req = apr_pcalloc(r->pool,sizeof(ra_request_rec));
	req->r = r;

	sreq = R_MakeExternalPtr(req,RA_REQUEST_REC_type,R_NilValue);

	SET_CLASS(sreq,RA_REQUEST_REC_class);

	return sreq;
}

/*
 * "$.request_rec" dispatcher
 */
SEXP RA_request_rec_idx(SEXP r, SEXP mbr)
{
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(r,RA_REQUEST_REC_type);
	char *member = CHAR(STRING_PTR(mbr)[0]);
	ra_struct_def_t *def = request_rec_def;

	if (!req) return R_NilValue;

	for (; def->member != NULL ; def++){
		if (strcmp(member,def->member) == 0){
			return (*def->fun)((void*)req->r,def->offset);
		}
	}

	return R_NilValue;
}

/*
 * "[[.request_rec" dispatcher
 *
 * we just need to ensure that r is a string
 */
SEXP RA_request_rec_idxc(SEXP r, SEXP mbr)
{
	if (IS_CHARACTER(mbr) && LENGTH(mbr) == 1)
		return RA_request_rec_idx(r,mbr);
	else
		return R_NilValue;
}

/*
 * names.request_rec dispatcher
 */
#define RA_REQUEST_REC_SIZE ((sizeof(request_rec_def) / sizeof(ra_struct_def_t)) - 1)
SEXP RA_request_rec_names(SEXP r)
{
	void *ptr = ra_unmarshall_pointer(r,RA_REQUEST_REC_type);
	SEXP names;
	ra_struct_def_t *def = request_rec_def;
	int i = 0;

	if (!ptr)  return R_NilValue;

	PROTECT(names = NEW_STRING(RA_REQUEST_REC_SIZE));
	for (; def->member != NULL ; def++){
		CHARACTER_DATA(names)[i++] = COPY_TO_USER_STRING( def->member );
	}
	UNPROTECT(1);

	return names;
}

SEXP RA_request_rec_length(SEXP r){
	SEXP len;
	PROTECT(len = NEW_INTEGER(1));
	INTEGER_DATA(len)[0] = RA_REQUEST_REC_SIZE;
	UNPROTECT(1);
	return len;
}

/*
 * "[[.apr_table dispatcher when index is numeric
 */
SEXP RA_apr_table_idxi(SEXP t,SEXP elem){
	apr_table_t *ptr = (apr_table_t *)ra_unmarshall_pointer(t,RA_APR_TABLE_type);
	int i =  INTEGER(AS_INTEGER(elem))[0];
	const apr_array_header_t *hdr;
	apr_table_entry_t *elt;
	SEXP val;

	hdr = apr_table_elts(ptr); if (!hdr) return R_NilValue;
	elt = (apr_table_entry_t *)hdr->elts;     if (!elt) return R_NilValue;

	/* Check index is within range based on R indexing */
	if (i < 1 || i > hdr->nelts) return R_NilValue;

	/* Convert index to c based indexing */
	i--;

	PROTECT(val = NEW_STRING(1));

	CHARACTER_DATA(val)[0] = COPY_TO_USER_STRING(elt[i].val);

	return val;


}

/*
 * "[[.apr_table" dispatcher when index is charac
 */
SEXP RA_apr_table_idxc(SEXP t, SEXP elem)
{
	if (IS_CHARACTER(elem) && LENGTH(elem) == 1)
		return RA_apr_table_idx(t,elem);
	else if ((IS_NUMERIC(elem) || IS_INTEGER(elem)) && LENGTH(elem) == 1)
		return RA_apr_table_idxi(t,elem);
	else
		return R_NilValue;
}

/*
 * "$.apr_table" dispatcher
 */
SEXP RA_apr_table_idx(SEXP t, SEXP elem)
{
	apr_table_t *ptr = (apr_table_t *)ra_unmarshall_pointer(t,RA_APR_TABLE_type);
	char *key = CHAR(STRING_PTR(elem)[0]);
	const char *val;
	SEXP sval;

	if (!ptr || !key) return R_NilValue;

	val = (char *)apr_table_get(ptr,key);

	if (!val) return R_NilValue;

	PROTECT(sval = NEW_STRING(1));
	CHARACTER_DATA(sval)[0] = COPY_TO_USER_STRING( val );
	UNPROTECT(1);
	return sval;
}

/*
 * names.apr_table dispatcher
 */
SEXP RA_apr_table_names(SEXP t)
{
	apr_table_t *ptr = (apr_table_t *)ra_unmarshall_pointer(t,RA_APR_TABLE_type);
	const apr_array_header_t *hdr;
	apr_table_entry_t *elt;
	int i;
	SEXP names;

	hdr = apr_table_elts(ptr); if (!hdr) return R_NilValue;
	elt = (apr_table_entry_t *)hdr->elts;         if (!elt) return R_NilValue;
	if (hdr->nelts <= 0) return R_NilValue;

	PROTECT(names = NEW_STRING(hdr->nelts));
	for (i = 0; i < hdr->nelts; i++){
		CHARACTER_DATA(names)[i] = COPY_TO_USER_STRING( elt[i].key );
	}
	UNPROTECT(1);
	return names;
}

/*
 * length.apr_table dispatcher
 */
SEXP RA_apr_table_length(SEXP t){
	apr_table_t *ptr = (apr_table_t *)ra_unmarshall_pointer(t,RA_APR_TABLE_type);
	SEXP intval;

	PROTECT(intval = NEW_INTEGER(1));
	INTEGER_DATA(intval)[0] = apr_table_elts(ptr)->nelts + 1;
	UNPROTECT(1);

	return intval;
}

/*
 * "$<-.apr_table" dispatcher
 */
SEXP RA_apr_table_set(SEXP t, SEXP header, SEXP value)
{
	apr_table_t *ptr = (apr_table_t *)ra_unmarshall_pointer(t,RA_APR_TABLE_type);
	char *key = CHAR(STRING_PTR(header)[0]);
	char *val = CHAR(STRING_PTR(value)[0]);

	if (!ptr || !key) return R_NilValue;

	apr_table_set(ptr,key,val);

	return t;
}

/* static SEXP empty_string(){
	SEXP str;
	PROTECT(str = NEW_STRING(1));
	CHARACTER_DATA(str)[0] = COPY_TO_USER_STRING( "" );
	UNPROTECT(1);
	return str;
} */

/*
 * apache.read
 */
SEXP RA_read(SEXP sr, SEXP slen){
	ra_request_rec *req; 
	request_rec *r;
	apr_size_t len, bpos, blen;
	apr_status_t rv;
	char *buf;
	SEXP str;
	apr_bucket *bucket;
	const char *data;

	req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);
	r = req->r;

	if (IS_INTEGER(slen) && LENGTH(slen) == 1){
		len =  (apr_size_t)INTEGER(slen)[0];
		if (len < 0) return R_NilValue;
		else if (len == 0) len = HUGE_STRING_LEN;
	} else {
		return R_NilValue;
	}

	/* See if apache.get_post() already parsed client request */
	if (req->parsed){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"Can't call apache.read when apache.get_post already called.");
		return R_NilValue;
	}

	/* Start reading the client request. this means that we can't parse with
	 * apache.get_post()
	 */
	req->read_start = 1;

	blen = len;
	buf = R_alloc(blen+1, sizeof(char)); /* +1 for NULL */
	bpos = 0;

	if (MR_bbin == NULL){
		MR_bbin = apr_brigade_create(r->pool, r->connection->bucket_alloc);
	}


	rv = ap_get_brigade(r->input_filters, MR_bbin, AP_MODE_READBYTES,
			APR_BLOCK_READ, len);

	if (rv != APR_SUCCESS) {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"apache.read: ap_get_brigade() returned an error!");
		return R_NilValue;
	}

	APR_BRIGADE_FOREACH(bucket, MR_bbin) {

		if (APR_BUCKET_IS_EOS(bucket)) {
			if (bpos == 0) { /* end of stream and no data , so return NULL */
				apr_brigade_cleanup(MR_bbin);
				return R_NilValue;
			} else { /* We've read some data, so go ahead and return it */
				break;
			}
		}

		/* We can't do much with this. */
		if (APR_BUCKET_IS_FLUSH(bucket)) {
			continue;
		}

		/* read */
		apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);
		memcpy(buf+bpos,data,len);
		bpos += len;
	}
	apr_brigade_cleanup(MR_bbin);

	buf[bpos] = '\0';
	PROTECT(str = NEW_STRING(1));
	CHARACTER_DATA(str)[0] = COPY_TO_USER_STRING( buf );
	UNPROTECT(1);

	return str;
}

/*
 * apache.readline
 */
SEXP RA_readline(SEXP sr, SEXP slen){
	ra_request_rec *req;
	request_rec *r;
	apr_size_t len, bpos, blen;
	apr_status_t rv;
	char *buf;
	SEXP str;
	apr_bucket *bucket;
	const char *data;

	req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);
	r = req->r;

	if (isInteger(slen) && LENGTH(slen) == 1){
		len = (apr_size_t)INTEGER(slen)[0];
		if (len < 0) return R_NilValue;
		else if (len == 0) len = HUGE_STRING_LEN;
	} else {
		return R_NilValue;
	}

	/* See if apache.get_post() already parsed client request */
	if (req->parsed){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"Can't call apache.readline after apache.get_post already called!");
		return R_NilValue;
	}

	/* Start reading the client request. this means that we can't parse with
	 * apache.get_post()
	 */
	req->read_start = 1;

	blen = len;
	buf = R_alloc(blen+1, sizeof(char)); /* +1 for NULL */
	bpos = 0;

	if (MR_bbin == NULL){
		MR_bbin = apr_brigade_create(r->pool, r->connection->bucket_alloc);
	}


	rv = ap_get_brigade(r->input_filters, MR_bbin, AP_MODE_GETLINE,
			APR_BLOCK_READ, 0);

	if (rv != APR_SUCCESS) {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"apache.read: ap_get_brigade() returned an error!");
		return R_NilValue;
	}

	APR_BRIGADE_FOREACH(bucket, MR_bbin) {

		if (APR_BUCKET_IS_EOS(bucket)) {
			if (bpos == 0) { /* end of stream and no data , so return NULL */
				apr_brigade_cleanup(MR_bbin);
				return R_NilValue;
			} else { /* We've read some data, so go ahead and return it */
				break;
			}
		}

		/* We can't do much with this. */
		if (APR_BUCKET_IS_FLUSH(bucket)) {
			continue;
		}

		/* read */
		apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);
		memcpy(buf+bpos,data,len);
		bpos += len;
	}
	apr_brigade_cleanup(MR_bbin);

	buf[bpos] = '\0';
	PROTECT(str = NEW_STRING(1));
	CHARACTER_DATA(str)[0] = COPY_TO_USER_STRING( buf );
	UNPROTECT(1);

	return str;
}

/*
 * apache.write
 */
SEXP RA_write(SEXP sr, SEXP str, SEXP flush){
	ra_request_rec *req;
	request_rec *r;

	req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);
	r = req->r;
	int i;

	if (!r) return ra_logical(FALSE);

	if (!IS_CHARACTER(str)){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"RApache: apache.write failed! argument not a string");
		return ra_logical(FALSE);
	}

	for (i = 0; i < LENGTH(str); i++){
		if (ap_fputs(r->output_filters,MR_bbout,CHAR(CHARACTER_DATA(str)[i])) != APR_SUCCESS)
			return ra_logical(FALSE);
	}

	if (IS_LOGICAL(flush) && asInteger(flush))
		ap_filter_flush(MR_bbout,r->output_filters);

	return ra_logical(TRUE);
}


/*
 * apache.get_args
 */
SEXP RA_get_args(SEXP sreq){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sreq,RA_REQUEST_REC_type);

	/* If we've already made the table, just hand it out again */
	if (req->args) return ra_make_apr_table_sexp(req->args);

	/* Don't parse if there aren't an GET variables to parse */
	if(!req->r->args) return R_NilValue;

	/* First call: parse and return */
	req->args = apr_table_make(req->r->pool,APREQ_DEFAULT_NELTS);
	apreq_parse_query_string(req->r->pool,req->args,req->r->args);

	return ra_make_apr_table_sexp(req->args);
}

typedef struct {
	SEXP files;
	SEXP names;
	int i;
} push_files_t ;

static int push_files(void *ft,const char *key, const char *val){
	push_files_t  *pf = (push_files_t *)ft;
	apreq_param_t *p = apreq_value_to_param(val);
	apr_file_t *f;
	apr_finfo_t finfo;
	const char *filename;
	SEXP file_elem, name_elem, str1, str2;

	f = apreq_brigade_spoolfile(p->upload);

	/* shouldn't ever happen, but... */
	if (f == NULL){
		filename = "";
	} else {
		apr_file_info_get(&finfo,APR_FINFO_SIZE,f);

		/* Only set tmp_name to the real tmp file location if
		 * its size is non-zero.
		 */
		filename = (finfo.size > 0)? finfo.fname : "";
	}

	PROTECT(file_elem = NEW_LIST(2));
	PROTECT(name_elem = NEW_STRING(2));
	PROTECT(str1 = NEW_STRING(1));
	PROTECT(str2 = NEW_STRING(1));

	SET_ELEMENT(str1,0,COPY_TO_USER_STRING(val));
	SET_ELEMENT(str2,0,COPY_TO_USER_STRING(filename));

	SET_ELEMENT(file_elem,0,str1);
	SET_ELEMENT(file_elem,1,str2);

	SET_ELEMENT(name_elem,0,COPY_TO_USER_STRING("name"));
	SET_ELEMENT(name_elem,1,COPY_TO_USER_STRING("tmp_name"));

	SET_NAMES(file_elem,name_elem);

	SET_ELEMENT(pf->files,pf->i,file_elem);
	SET_ELEMENT(pf->names,pf->i,COPY_TO_USER_STRING(key));

	pf->i += 1;

	UNPROTECT(4);

	return 1;
}
/*
 * apache.get_post
 */
SEXP RA_get_post(SEXP sreq){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sreq,RA_REQUEST_REC_type);
	apreq_parser_t *psr;
	apr_bucket_brigade *bb;
	const char *tmpdir;
	apr_status_t s;
	const apr_table_t *uploads;
	SEXP filenames;
	int nfiles;

	if (req->read_start) {
		/* If we've already started reading with either apache.read() or
		 * apache.readline() then don't try to parse at all.
		 */
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,req->r,"Can't call apache.get_post since either apache.read or apache.readline already called");

		return R_NilValue;
	} else if (req->parsed){
		/* We've already parsed the input, just hand out the result */
		if (req->post) 
			return ra_make_apr_table_sexp(req->post);
		else 
			return R_NilValue;; /* will be R_NilValue */
	} else {
		/* First call, parse and return */
		req->parsed=1;
	}

	/* But don't parse if method was not POST */
	if (strcmp(req->r->method,"POST") != 0) {
		req->post = NULL;
		req->uploads = R_NilValue;
		return R_NilValue;
	}

	req->post = apr_table_make(req->r->pool, APREQ_DEFAULT_NELTS);
	apr_temp_dir_get(&tmpdir,req->r->pool);
	psr = apreq_parser_make(
			req->r->pool,
			req->r->connection->bucket_alloc,
			apr_table_get(req->r->headers_in,"Content-Type"),
			apreq_parser(apr_table_get(req->r->headers_in,"Content-Type")),
			0, /* brigade_limit */
			tmpdir,
			NULL,
			NULL);
	bb = apr_brigade_create(req->r->pool,req->r->connection->bucket_alloc);
	while((s = ap_get_brigade(req->r->input_filters,bb,AP_MODE_READBYTES,APR_BLOCK_READ,HUGE_STRING_LEN)) == APR_SUCCESS){ 
		apreq_parser_run(psr,req->post,bb);
		if (APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(bb))) break;
		apr_brigade_cleanup(bb);
	}
	apr_brigade_cleanup(bb);

	/* Now go ahead and set req->uploads*/
	uploads = apreq_uploads(req->post,req->r->pool);
	nfiles = apr_table_elts(uploads)->nelts;

	if (nfiles){
		push_files_t PFTfiles;
		PROTECT(req->uploads = NEW_LIST(nfiles));
		PROTECT(filenames = NEW_STRING(nfiles));

		PFTfiles.files = req->uploads;
		PFTfiles.names = filenames;
		PFTfiles.i = 0;
		apr_table_do(push_files,(void *)&PFTfiles,uploads,NULL);
		SET_NAMES(req->uploads,filenames);
		UNPROTECT(2);
	} else {
		req->uploads = R_NilValue;
	}

	return ra_make_apr_table_sexp(req->post);
}

/*
 * apache.get_cookies
 */
SEXP RA_get_cookies(SEXP sreq){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sreq,RA_REQUEST_REC_type);
	const char *cookies;

	if (req->cookies) return ra_make_apr_table_sexp(req->cookies);

	cookies = apr_table_get(req->r->headers_in, "Cookie");

	if (cookies == NULL) return R_NilValue;

	req->cookies = apr_table_make(req->r->pool,APREQ_DEFAULT_NELTS);
	apreq_parse_cookie_header(req->r->pool,req->cookies,cookies);

	return ra_make_apr_table_sexp(req->cookies);
}

/*
 * apache.get_uploads
 */
SEXP RA_get_uploads(SEXP sreq){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sreq,RA_REQUEST_REC_type);

	if (req->read_start) {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,req->r,"RApache: Can't call apache.get_uploads() since either apache.read() or apache.readline() already called");
		return R_NilValue;
	} else if (!req->parsed) {
		RA_get_post(sreq);
	}

	return req->uploads;
}

/*
 * apache.add_common_vars
 */
SEXP RA_ap_add_common_vars(SEXP sr){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);

	ap_add_cgi_vars(req->r);
	ap_add_common_vars(req->r);

	return R_NilValue;
}

/*
 * apache.log_errors
 */
SEXP RA_log_error(SEXP sr, SEXP msg, SEXP level){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);

	if (IS_CHARACTER(msg) && LENGTH(msg) == 1 && IS_INTEGER(level) && LENGTH(level) == 1)
		ap_log_rerror(APLOG_MARK, asInteger(level), 0, req->r, "%s", 
				CHAR(CHARACTER_DATA(msg)[0]));

	return R_NilValue;
}

/*
 * apache.allow_methods
 */
SEXP RA_allow_methods(SEXP r, SEXP replace, SEXP methods){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(r,RA_REQUEST_REC_type);
	int len,i,method;

	if (!IS_LOGICAL(replace)){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,req->r,"apache.allow_methods: replace argument must be LOGICAL!");
		return R_NilValue;
	}

	if (!IS_LIST(methods) || LENGTH(methods) == 0){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,req->r,"apache.allow_methods: argument must be LIST!");
		return R_NilValue;
	}

	len = LENGTH(methods);

	ap_allow_methods(req->r,(asInteger(replace) == REPLACE_ALLOW),
			ap_method_name_of(req->r->pool,INTEGER_DATA(methods)[0]), NULL);

	for (i = 1; i < len; i++){
		method = INTEGER_DATA(VECTOR_ELT(methods,i))[0];
		ap_allow_methods(req->r,(asInteger(replace) == MERGE_ALLOW),
				ap_method_name_of(req->r->pool,method), NULL);
	}

	return R_NilValue;
}

SEXP RA_gdlib_ioctx(SEXP sr){
#ifndef HAVE_GDLIB 
	return R_NilValue;
#else
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);
	request_rec *r;
	gdIOCtxPtr ioctx;

	r = req->r;

	ioctx = (gdIOCtxPtr)apr_pcalloc(r->pool,sizeof(ra_gd_ioctx));

	ioctx->getC = ra_gd_getC;
	ioctx->getBuf = ra_gd_getBuf;
	ioctx->putC = ra_gd_putC;
	ioctx->putBuf = ra_gd_putBuf;
	ioctx->seek = ra_gd_seek;
	ioctx->tell = ra_gd_tell;
	ioctx->gd_free = ra_gd_free;
	((ra_gd_ioctx*)ioctx)->r = r;
	;
	return R_MakeExternalPtr(ioctx,install("ra_gd_ioctx"),R_NilValue);
#endif
}
#ifdef HAVE_GDLIB
static int ra_gd_getC(gdIOCtxPtr ioctx){ return 0; }
static int ra_gd_getBuf(gdIOCtxPtr ioctx, void *buf, int i){ return 0; }
static void ra_gd_putC(gdIOCtxPtr ioctx, int c){
	request_rec *r = ((ra_gd_ioctx*)ioctx)->r;
	ap_fputc(r->output_filters,MR_bbout,(char)c);
}
static int ra_gd_putBuf(gdIOCtxPtr ioctx, const void *buf, int i){
	request_rec *r = ((ra_gd_ioctx*)ioctx)->r;
	ap_fwrite(r->output_filters,MR_bbout,buf,i);
	return i;
}
static int ra_gd_seek(gdIOCtxPtr ioctx, const int i){ return 0; }
static long ra_gd_tell(gdIOCtxPtr ioctx){ return 0l; }
static void ra_gd_free(gdIOCtxPtr ioctx){ }
#endif

SEXP RA_ap_set_content_type(SEXP sr, SEXP stype){
	ra_request_rec *req = (ra_request_rec *)ra_unmarshall_pointer(sr,RA_REQUEST_REC_type);

	if (req == NULL || !IS_CHARACTER(stype) || LENGTH(stype) != 1){
		return R_NilValue;
	}

	ap_set_content_type( req->r, apr_pstrdup(req->r->pool,CHAR(CHARACTER_DATA(stype)[0])) );

	return stype;
}

static void *ra_unmarshall_pointer(SEXP ptr, SEXP type)
{

	if (TYPEOF(ptr) != EXTPTRSXP){ 
		error("ptr Not an External Pointer!");
		return NULL;
	}
	if (R_ExternalPtrTag(ptr) != type){
		error("External Pointer Type Mismatch");
		return NULL;
	}
	return R_ExternalPtrAddr(ptr);
}

static SEXP ra_make_apr_table_sexp(apr_table_t *t){
	SEXP req;

	if (t == NULL) return R_NilValue;

	req = R_MakeExternalPtr(t,RA_APR_TABLE_type,R_NilValue);
	SET_CLASS(req,RA_APR_TABLE_class);
	return req;
}

static SEXP ra_getstrmbr(void *s, int offset)
{
	SEXP str;
	char *ptr;

	if (s == NULL) return R_NilValue;
	ptr = *(char **)(s+offset);
	if (ptr == NULL) return R_NilValue;

	/* Now create SEXP to hold string */
	PROTECT(str = NEW_STRING(1));
	CHARACTER_DATA(str)[0] = COPY_TO_USER_STRING( ptr );
	UNPROTECT(1);

	return str;
}

static SEXP ra_getintmbr(void *s, int offset)
{
	SEXP intval;

	if (s == NULL) return R_NilValue;

	/* Now create SEXP to hold string */
	PROTECT(intval = NEW_INTEGER(1));
	INTEGER_DATA(intval)[0] = *((int *)(s+offset));
	UNPROTECT(1);

	return intval;
}

static SEXP ra_getlglmbr(void *s, int offset)
{
	SEXP intval;

	if (s == NULL) return R_NilValue;

	PROTECT(intval = NEW_LOGICAL(1));
	LOGICAL_DATA(intval)[0] = (*((int *)(s+offset)))?TRUE:FALSE;
	UNPROTECT(1);

	return intval;
}

static SEXP ra_gettablembr(void *r, int offset)
{
	apr_table_t *t = *(apr_table_t **)(r+offset);
	return ra_make_apr_table_sexp(t);

}

static SEXP ra_getoffmbr(void *r, int offset)
{
	apr_off_t t = *(apr_off_t *)(r+offset);
	SEXP dval;
	PROTECT(dval = NEW_NUMERIC(1));
	NUMERIC_DATA(dval)[0] = (double)t;
	UNPROTECT(1);

	return dval;
}

static SEXP ra_gettimembr(void *r, int offset)
{
	apr_time_t t = *(apr_time_t *)(r+offset);
	SEXP intval,class;

	/* Now create SEXP to hold string */
	PROTECT(intval = NEW_INTEGER(1));
	INTEGER_DATA(intval)[0] = (int)apr_time_sec(t);
	UNPROTECT(1);

	PROTECT(class = NEW_STRING(2));
	CHARACTER_DATA(class)[0] = COPY_TO_USER_STRING( "POSIXt" );
	CHARACTER_DATA(class)[1] = COPY_TO_USER_STRING( "POSIXct" );
	UNPROTECT(1);

	SET_CLASS(intval,class);

	return intval;
}

/* static SEXP ra_getrealmbr(void *r, int offset)
{
	return R_NilValue;
} */

static SEXP ra_logical(int logical)
{
	SEXP slog;

	PROTECT(slog = NEW_LOGICAL(1));
	LOGICAL_DATA(slog)[0] = logical?TRUE:FALSE;
	UNPROTECT(1);

	return slog;
}

/* static SEXP ra_not_implemented(void *r, int offset){return R_NilValue;} */

static void ra_define_types_classes()
{
	RA_REQUEST_REC_type = install("RA_REQUEST_REC_type");
	RA_APR_TABLE_type = install("RA_APR_TABLE_type");

	PROTECT(RA_REQUEST_REC_class = NEW_STRING(1));
	CHARACTER_DATA(RA_REQUEST_REC_class)[0] = COPY_TO_USER_STRING( "ra_request_rec" );

	PROTECT(RA_APR_TABLE_class = NEW_STRING(1));
	CHARACTER_DATA(RA_APR_TABLE_class)[0] = COPY_TO_USER_STRING( "apr_table" );

	UNPROTECT(2);

	R_PreserveObject(RA_REQUEST_REC_class);
	R_PreserveObject(RA_APR_TABLE_class);
}

#define CALLDEF(name, n)  { #name, (DL_FUNC) &name, n }
R_CallMethodDef callMethods[] = 
{
	CALLDEF(RA_request_rec_idx,2),
	CALLDEF(RA_request_rec_idxc,2),
	CALLDEF(RA_request_rec_names,1),
	CALLDEF(RA_request_rec_length,1),
	CALLDEF(RA_apr_table_idx,2),
	CALLDEF(RA_apr_table_idxc,2),
	CALLDEF(RA_apr_table_names,1),
	CALLDEF(RA_apr_table_length,1),
	CALLDEF(RA_apr_table_set,3),

	CALLDEF(RA_read,2),
	CALLDEF(RA_readline,2),
	CALLDEF(RA_write,3),
	CALLDEF(RA_get_args,1),
	CALLDEF(RA_get_post,1),
	CALLDEF(RA_get_cookies,1),
	CALLDEF(RA_get_uploads,1),
	CALLDEF(RA_ap_add_common_vars,1),
	CALLDEF(RA_log_error,3),
	CALLDEF(RA_allow_methods,3),
	CALLDEF(RA_ap_set_content_type,2),
	CALLDEF(RA_gdlib_ioctx,1),


	{NULL,NULL, 0}
};

void R_init_RApache(DllInfo *dll)
{
#ifndef RCHECK
	apr_pool_create(&RApache_pool, NULL);
	apreq_initialize(RApache_pool);
	ra_define_types_classes();
	RA_new_request_rec = &ra_new_request_rec;
	/* R_useDynamicSymbols(dll, FALSE); */
#endif /* RCHECK */
	R_registerRoutines(dll,NULL,callMethods,NULL,NULL);
}

void R_unload_RApache(DllInfo *dll)
{
	apr_pool_destroy(RApache_pool);
}
