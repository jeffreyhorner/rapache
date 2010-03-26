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
/*
 * $Id$
 */

/*************************************************************************
 *
 * Headers and macros
 *
 *************************************************************************/
#define MOD_R_VERSION "1.1.9"
#define SVNID "$Id$"
#include "mod_R.h" 

#include <sys/types.h>
#include <unistd.h>

/*
 * DARWIN's dyld.h defines an enum { FALSE TRUE} which conflicts with R
 */
#ifdef DARWIN
#define ENUM_DYLD_BOOL
enum DYLD_BOOL{ DYLD_FALSE, DYLD_TRUE};
#endif

/*
 * Apache Server headers
 */ 
#include "httpd.h"
#include "http_log.h"
#include "http_config.h"
#include "http_protocol.h"
#include "util_filter.h"
#include "util_script.h"
#include "ap_mpm.h"

/*
 * Apache Portable Runtime
 */
#include "apr.h"
#include "apr_errno.h"
#include "apr_strings.h"
#include "apr_env.h"
#include "apr_pools.h"
#include "apr_hash.h"
#include "apr_thread_mutex.h"

/*
 * libapreq headers
 */
#include "apreq.h"
#include "apreq_cookie.h"
#include "apreq_parser.h"
#include "apreq_param.h"
#include "apreq_util.h"


/*
 * R headers
 */
#include <R.h>
#include <Rversion.h>
#include <Rinternals.h>
#include <Rdefines.h>


#define R_INTERFACE_PTRS
#define CSTACK_DEFNS
/* 
 * APR pulls in stdint.h which defines uintptr_t, so we need to tell R
 * headers that. R defines uintptr_t slightly differently than 
 * stdint.h, although they *should* be the same size
 */
#define HAVE_UINTPTR_T
#include <Rinterface.h>

#include <Rembedded.h>
#include <R_ext/Parse.h>
#include <R_ext/Callbacks.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Memory.h>

/*************************************************************************
 *
 * RApache types
 *
 *************************************************************************/
typedef enum {
	R_HANDLER = 1,
	R_SCRIPT,
	R_INFO
} RApacheHandlerType;

typedef struct {
	char *file;
	char *function;
	char *package;
	char *handlerKey;
	int preserveEnv;
} RApacheDirective;

typedef struct {
	SEXP exprs;
	SEXP envir;
	apr_time_t mtime;
} RApacheParsedFile;

typedef struct {
	SEXP expr;
	SEXP envir;
	RApacheParsedFile *parsedFile;
	RApacheDirective *directive;
} RApacheHandler;

typedef struct {
	request_rec *r;
	int postParsed;
	int readStarted;
	apr_table_t *argsTable;
	apr_table_t *postTable;
	apr_table_t *cookiesTable;
	SEXP filesVar;
	SEXP serverVar;
	int outputErrors;
	char *errorPrefix;
	char *errorSuffix;
} RApacheRequest;

/*************************************************************************
 *
 * Globals; prefixed with MR_ (if R ever becomes threaded, or can at least
 * run multiple interpreters at once, we'll bundle these up into contexts).
 *
 *************************************************************************/

/* Current request_rec */
static RApacheRequest MR_Request = { NULL, 0, 0, NULL, NULL, NULL, NULL , NULL, -1, NULL, NULL};

/* Option to turn on errors in output */
static int MR_OutputErrors = 0;
static const char *MR_ErrorPrefix = "<div style=\"background-color:#eee; padding: 20px 20px 20px 20px; font-size: 14pt; border: 1px solid red;\"><code>";
static const char *MR_ErrorSuffix = "</code></div>";

/* Number of times apache has parsed config files; we'll do stuff on second pass */
static int MR_ConfigPass = 1;

/* Don't start R more than once, if the flag is 1 it's already started */
static int MR_InitStatus = 0;

/* 
 * Child pid. If we fork() we don't want to do certain
 * cleanup steps, especially on cgi fork, so we cache the original
 * pid.
 */
static unsigned long MR_pid;

/* Per-child memory for configuration */
static apr_pool_t *MR_Pool = NULL;
static apr_hash_t *MR_HandlerCache = NULL;
static apr_hash_t *MR_ParsedFileCache = NULL;
static apr_table_t *MR_OnStartup = NULL;

/*
 * Bucket brigades for reading and writing to client
 * TODO: stuff these into MR_Request
 */
static apr_bucket_brigade *MR_BBin;
static apr_bucket_brigade *MR_BBout;

/*
 * RApache environment
 */
static SEXP MR_RApacheEnv;

/*
 * RApache code; evaluated in the above environment.
 */

static const char MR_RApacheSource[] = "\
setHeader <- function(header,value) .Call('RApache_setHeader',header,value)\n\
setContentType <- function(type) .Call('RApache_setContentType',type)\n\
setCookie <- function(name=NULL,value='',expires=NULL,path=NULL,domain=NULL,...){\n\
	args <- list(...)\n\
	therest <- ifelse(length(args)>0,paste(paste(names(args),args,sep='='),collapse=';'),'')\n\
	.Call('RApache_setCookie',name,value,expires,path,domain,therest)}\n\
urlEncode <- function(str) .Call('RApache_urlEnDecode',str,TRUE)\n\
urlDecode <- function(str) .Call('RApache_urlEnDecode',str,FALSE)\n\
RApacheInfo <- function() .Call('RApache_RApacheInfo')\n\
sendBin <- function(object, con=stdout(), size=NA_integer_, endian=.Platform$endian){\n\
	swap <- endian != .Platform$endian\n\
	if (!is.vector(object) || mode(object) == 'list') stop('can only write vector objects')\n\
	.Call('RApache_sendBin',object,size,swap)}\n\
RApacheOutputErrors <- function(status=TRUE,prefix=NULL,suffix=NULL) .Call('RApache_outputErrors',status,prefix,suffix)";

/*
 * CGI Expression list. These are evaluated in the RApache environment before every request.
 */
static SEXP MR_CGIexprs;

/*
 * CGI Variables. Each tuple is passed to delayedAssign() and evaluated in the RApache environment
 * before every request.
 */
static const char *MR_CGIvars[] = {
	"GET", "RApache_parseGet",
	"COOKIES", "RApache_parseCookies",
	"POST", "RApache_parsePost",
	"FILES", "RApache_parseFiles",
	"SERVER", "RApache_getServer",
	NULL
};

/*
 * Global Thread Mutex
 */
static apr_thread_mutex_t *MR_mutex = NULL;

/*************************************************************************
 *
 * Function declarations
 *
 * AP_* functions are called by apache code base
 *
 *************************************************************************/

/*
 * Module functions
 */
static void *AP_create_dir_cfg(apr_pool_t *p, char *dir);
static void *AP_merge_dir_cfg(apr_pool_t *p, void *parent, void *new);
static void *AP_create_srv_cfg(apr_pool_t *p, server_rec *s);
static void *AP_merge_srv_cfg(apr_pool_t *p, void *parent, void *new);
static void AP_register_hooks (apr_pool_t *p);

/*
 * Command functions
 */
static const char *AP_cmd_RHandler(cmd_parms *cmd, void *conf, const char *handler);
static const char *AP_cmd_RFileHandler(cmd_parms *cmd, void *conf, const char *handler);
static const char *AP_cmd_REvalOnStartup(cmd_parms *cmd, void *conf, const char *evalstr);
static const char *AP_cmd_RSourceOnStartup(cmd_parms *cmd, void *conf, const char *evalstr);
static const char *AP_cmd_ROutputErrors(cmd_parms *cmd, void *mconfig);
static const char *AP_cmd_RPreserveEnv(cmd_parms *cmd, void *mconfig);

/*
 * Module Hook functions
 */
static int AP_hook_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s);
static void AP_hook_child_init(apr_pool_t *p, server_rec *s);
static int AP_hook_request_handler (request_rec *r);

/*
 * Exit callback
 */
static apr_status_t AP_child_exit(void *data);

/*
 * R interface callbacks
 */
static void Suicide(const char *s){ };
static void ShowMessage(const char *s){ };
static int ReadConsole(const char *, unsigned char *, int, int);
static void WriteConsoleEx(const char *, int, int);
static void NoOpConsole(){ };
static void NoOpBusy(int i) { };
static void NoOpCleanUp(SA_TYPE s, int i, int j){ };
static int NoOpShowFiles(int i, const char **j, const char **k, const char *l, Rboolean b, const char *c){ return 1;};
static int NoOpChooseFile(int i, char *b,int s){ return 0;};
static int NoOpEditFile(const char *f){ return 0;};
static void NoOpHistoryFun(SEXP a, SEXP b, SEXP c, SEXP d){ };

/*
 * The Rest.
 */
static void init_config_pass(apr_pool_t *p);
static void init_R(apr_pool_t *);
static int call_fun1str( char *funstr, char *argstr);
static int decode_return_value(SEXP ret);
static int SetUpRequest(const request_rec *);
static void EmptyRequest(void);
static void TearDownRequest(int flush);
static RApacheHandler *GetHandlerFromRequest(const request_rec *r);
static int RApacheResponseError(char *msg);
static void RApacheError(char *msg);
static void InitTempDir(apr_pool_t *p);
static void RegisterCallSymbols();
static SEXP NewLogical(int tf);
static SEXP NewInteger(int tf);
static SEXP NewEnv(SEXP enclos);
static int ExecRCode(const char *code,SEXP env, int *error);
static SEXP ExecFun1Arg(SEXP fun, SEXP arg);
static SEXP ParseFile(const char *file, apr_pool_t *pool, apr_size_t fsize, ParseStatus *);
static int PrepareFileExprs(RApacheHandler *h, const request_rec *r, int *fileParsed);
static int PrepareHandlerExpr(RApacheHandler *h, const request_rec *r, int handlerType);
static SEXP EvalExprs(SEXP exprs, SEXP env, int *error); /* more than one expression evaluator*/
static void StartRprof();
static void StopRprof();
static void InitRApacheEnv();
static void InitRApachePool();
static int OnStartupCallback(void *rec, const char *key, const char *value);
static SEXP MyfindFun(SEXP fun, SEXP envir);
static SEXP MyfindFunInPackage(SEXP fun, char *package);
static int RApacheInfo();
static SEXP AprTableToList(apr_table_t *);
static void InitCGIexprs();
static void InjectCGIvars(SEXP env);
static void ResetEnclosure(RApacheHandler *h);

/*************************************************************************
 *
 * Command array
 *
 *************************************************************************/

static const command_rec MR_cmds[] = {
	AP_INIT_TAKE1("RHandler", AP_cmd_RHandler, NULL, OR_OPTIONS, "R function to handle request. "),
	AP_INIT_TAKE1("RFileHandler", AP_cmd_RFileHandler, NULL, OR_OPTIONS, "File containing R code or function to handle request."),
	AP_INIT_TAKE1("REvalOnStartup", AP_cmd_REvalOnStartup, NULL, OR_OPTIONS,"R expressions to evaluate on start."),
	AP_INIT_TAKE1("RSourceOnStartup", AP_cmd_RSourceOnStartup, NULL, OR_OPTIONS,"File containing R expressions to evaluate on start."),
	AP_INIT_NO_ARGS("ROutputErrors", AP_cmd_ROutputErrors, NULL, OR_OPTIONS, "Option to print error messages to output."),
	AP_INIT_NO_ARGS("RPreserveEnv", AP_cmd_RPreserveEnv, NULL, OR_OPTIONS, "Option to preserve handler environment across requests."),
	{ NULL},
};

/*************************************************************************
 *
 * Module definition
 *
 *************************************************************************/
module AP_MODULE_DECLARE_DATA R_module =
{
	STANDARD20_MODULE_STUFF,
	AP_create_dir_cfg,        /* dir config creater */
	AP_merge_dir_cfg,         /* dir merger --- default is to override */
	AP_create_srv_cfg,        /* server config */
	AP_merge_srv_cfg,         /* merge server config */
	MR_cmds,                  /* table of config file commands */
	AP_register_hooks,        /* register hooks */
};

/*************************************************************************
 *
 * Module functions: called by apache code base
 *
 *************************************************************************/

static void *AP_create_dir_cfg(apr_pool_t *p, char *dir){
	RApacheDirective *cfg;
	cfg = (RApacheDirective *)apr_pcalloc(p,sizeof(RApacheDirective));
	return (void *)cfg;
}
/*
void print_cfg(char *fname, char *cname1, RApacheDirective *c1, char *cname2, RApacheDirective *c2){
	printf("CALLED %s\n",fname);

	printf("%s:\n",cname1);
	printf("\tfile: %s\n",c1->file);
	printf("\tfunction: %s\n",c1->function);
	printf("\tpackage: %s\n",c1->package);
	printf("\thandlerKey: %s\n",c1->handlerKey);
	printf("\tpreserveEnv: %d\n",c1->preserveEnv);

	if (cname2){
		printf("%s:\n",cname2);
		printf("\tfile: %s\n",c2->file);
		printf("\tfunction: %s\n",c2->function);
		printf("\tpackage: %s\n",c2->package);
		printf("\thandlerKey: %s\n",c2->handlerKey);
		printf("\tpreserveEnv: %d\n",c2->preserveEnv);
	}

} */

void *AP_merge_dir_cfg(apr_pool_t *pool, void *parent, void *new){
	RApacheDirective *c;
	RApacheDirective *p = (RApacheDirective *)parent;
	RApacheDirective *n = (RApacheDirective *)new;

	/* print_cfg("AP_merge_dir_cfg","Parent",p,"New",n); */

	c = (RApacheDirective *)apr_pcalloc(pool,sizeof(RApacheDirective));

	/* add new config stuff, overriding parent */
	memcpy(c,n,sizeof(RApacheDirective));

	return (void *)c;
}

/* first mod_R.c function called when apache starts */
void *AP_create_srv_cfg(apr_pool_t *p, server_rec *s){
	RApacheDirective *c;

	init_config_pass(s->process->pool);

	c = (RApacheDirective *)apr_pcalloc(p,sizeof(RApacheDirective));

	return (void *)c;
}

void *AP_merge_srv_cfg(apr_pool_t *pool, void *parent, void *new){
	RApacheDirective *c;
	RApacheDirective *p = (RApacheDirective *)parent;
	RApacheDirective *n = (RApacheDirective *)new;

	/* print_cfg("AP_merge_srv_cfg","Parent",p,"New",n); */

	c = (RApacheDirective *)apr_pcalloc(pool,sizeof(RApacheDirective));

	/* add new config stuff, overriding parent */
	memcpy(c,n,sizeof(RApacheDirective));

	return (void *)c;
}

static void AP_register_hooks (apr_pool_t *p)
{
	ap_hook_post_config(AP_hook_post_config, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_child_init(AP_hook_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(AP_hook_request_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/*************************************************************************
 *
 * Command functions: called by apache code base
 *
 *************************************************************************/

static const char *AP_cmd_RHandler(cmd_parms *cmd, void *conf, const char *handler){
	const char *part, *function;
	RApacheDirective *c = (RApacheDirective *)conf;
	InitRApachePool();

	if (ap_strchr(handler,'/')){
		fprintf(stderr,"\n\tWARNING! %s seems to contain a file. If this is true, then use the RFileHandler directive instead.\n",handler);
	}

	c->handlerKey = apr_pstrdup(cmd->pool,handler);
	part = ap_strstr(handler,"::");
	if (part) {
		c->package = apr_pstrmemdup(cmd->pool,handler,part - handler);
		apr_table_add(MR_OnStartup,"package",c->package);
		function = part + 2;
	} else {
		function = handler;
	}
	c->function = apr_pstrdup(cmd->pool,function);

	return NULL;
}

static const char *AP_cmd_RFileHandler(cmd_parms *cmd, void *conf, const char *handler){
	const char *part;
	RApacheDirective *c = (RApacheDirective *)conf;
	apr_finfo_t finfo;
	InitRApachePool();

	c->handlerKey = apr_pstrdup(cmd->pool,handler);

	part = ap_strstr(handler,"::");
	if (part) {
		c->file = apr_pstrmemdup(cmd->pool,handler,part - handler);
		c->function = apr_pstrdup(cmd->pool,part + 2);
	} else {
		c->file = apr_pstrdup(cmd->pool,handler);
	}

	if (apr_stat(&finfo,c->file,APR_FINFO_TYPE,cmd->pool) != APR_SUCCESS){
		return apr_psprintf(cmd->pool,"RFileHandler: %s file not found!",c->file);
	}
	return NULL;
}

static const char *AP_cmd_REvalOnStartup(cmd_parms *cmd, void *conf, const char *evalstr){
	InitRApachePool();
	apr_table_add(MR_OnStartup,"eval",evalstr);
	return NULL;
}

static const char *AP_cmd_RSourceOnStartup(cmd_parms *cmd, void *conf, const char *file){
	InitRApachePool();
	if (MR_ConfigPass == 1){
		apr_finfo_t finfo;
		if (apr_stat(&finfo,file,APR_FINFO_TYPE,cmd->pool) != APR_SUCCESS){
			return apr_psprintf(cmd->pool,"RSourceFile: %s file not found!",file);
		}
		return NULL;
	}
	apr_table_add(MR_OnStartup,"file",file);
	return NULL;
}

static const char *AP_cmd_ROutputErrors(cmd_parms *cmd, void *conf){
	MR_OutputErrors = 1;
	return NULL;
}

static const char *AP_cmd_RPreserveEnv(cmd_parms *cmd, void *conf){
	RApacheDirective *c = (RApacheDirective *)conf;
	c->preserveEnv = 1;
	return NULL;
}

/*************************************************************************
 *
 * Hook functions: called by apache code base
 *
 *************************************************************************/

static int AP_hook_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s){
	ap_add_version_component(pconf,apr_psprintf(pconf,"mod_R/%s",MOD_R_VERSION));
	ap_add_version_component(pconf,apr_psprintf(pconf,"R/%s.%s",R_MAJOR,R_MINOR));
	return OK;
}

static void AP_hook_child_init(apr_pool_t *p, server_rec *s){
	MR_pid=(unsigned long)getpid();
	init_R(p);
	apr_pool_cleanup_register(p, p, AP_child_exit, AP_child_exit);
}

static void ResetEnclosure(RApacheHandler *h){
	SET_CLOENV(CAR(h->expr),ENCLOS(MR_RApacheEnv));
	SET_ENCLOS(MR_RApacheEnv,R_GlobalEnv);
}

static int AP_hook_request_handler (request_rec *r)
{
	RApacheHandlerType handlerType = 0;
	RApacheHandler *h;
	SEXP ret;
	int error=1,fileParsed=1;
	apr_status_t rv;

	/* Only handle our handlers */
	if (strcmp(r->handler,"r-handler")==0) handlerType = R_HANDLER;
	else if (strcmp(r->handler,"r-script")==0) handlerType = R_SCRIPT;
	else if (strcmp(r->handler,"r-info")==0) handlerType = R_INFO;
	else return DECLINED;

	if (!SetUpRequest(r)) return HTTP_INTERNAL_SERVER_ERROR;

	if (handlerType == R_INFO){
		int val = RApacheInfo();
		TearDownRequest(1);
		return val;
	}

	/* Get cached handler object from request */
	h = GetHandlerFromRequest(r);

	if (!h) return RApacheResponseError(NULL);

	/* Prepare calling environment */
	SET_ENCLOS(MR_RApacheEnv,R_GlobalEnv);


	/* Prepare file if needed */
	if (h->directive->file){
		fileParsed = 1;
		if (!PrepareFileExprs(h,r,&fileParsed)) return RApacheResponseError(NULL);
		
		/* Preserved environments are per file */
		if (h->directive->preserveEnv){
		   if (!h->parsedFile->envir) R_PreserveObject(h->parsedFile->envir = NewEnv(MR_RApacheEnv));
		   h->envir = h->parsedFile->envir;
		} else {
		   h->envir = NewEnv(MR_RApacheEnv);
		}
		InjectCGIvars(h->envir);

		/* Do we need to eval parsed file?
		 * Yes, if env is not preserved.
		 * Yes, if file newly parsed.
		 * Yes, if there's no function to eval.
		 */
		if (!h->directive->preserveEnv || fileParsed || !h->directive->function){
			PROTECT(h->envir);
			PROTECT(h->parsedFile->exprs); /* already preserved in PrepareFileExprs, but... */
			error = 1;
			ret = EvalExprs(h->parsedFile->exprs,h->envir,&error);
			UNPROTECT(2);
			if (error) return RApacheResponseError(apr_psprintf(r->pool,"Error evaluating %s!\n",h->directive->file));
		}
	} else {

		/* No file means a function found in the either a global 
		 * environment or an attached package.
		 */ 
		h->envir = NewEnv(MR_RApacheEnv);
		InjectCGIvars(h->envir);
	}

	/* Eval handler expression if set */
	if (h->directive->function){
		if (!PrepareHandlerExpr(h,r,handlerType)) {

			/* Only need to reset enclosure when no file present */
			if (!h->directive->file) ResetEnclosure(h);
			return RApacheResponseError(NULL);
		}
		PROTECT(h->envir);
		PROTECT(h->expr);
		error=1;
		ret = R_tryEval(h->expr,h->envir,&error);
		UNPROTECT(2);

		/* Only need to reset enclosure when no file present */
		if (!h->directive->file) ResetEnclosure(h);
		
		if (error) return RApacheResponseError(apr_psprintf(r->pool,"Error calling function %s!\n",h->directive->function));
	}


	if (IS_INTEGER(ret) && LENGTH(ret) == 1){
		TearDownRequest(1);
		return asInteger(ret);
	} else if (inherits(ret,"try-error")){
		return RApacheResponseError(apr_psprintf(r->pool,"Function %s returned an object of 'try-error'. Returning HTTP response code 500.\n",h->directive->function));
	} else {
		TearDownRequest(1);
		return DONE; /* user didn't specify return code, so presume done.*/
	}
}

static apr_status_t AP_child_exit(void *data){
	/* R_dot_Last(); */
	unsigned long pid;

	pid = (unsigned long)getpid();

	/* Only run if we've actually initialized AND
	 * we haven't forked (for mod_cgi).
	 */
	if (MR_InitStatus && MR_pid == pid){
		MR_InitStatus = 0;
		/* R_RunExitFinalizers(); */
		/* Rf_CleanEd(); */
		/* KillAllDevices(); */
	}

	if (MR_Pool) {
		apr_pool_destroy(MR_Pool);
		MR_Pool = NULL;
	}

	return APR_SUCCESS;
}

/*************************************************************************
 *
 * R interface callbacks
 *
 *************************************************************************/
static void WriteConsoleEx(const char *buf, int size, int error){
	if (MR_Request.r){
		if (!error) ap_fwrite(MR_Request.r->output_filters,MR_BBout,buf,size);
		else RApacheError(apr_pstrmemdup(MR_Request.r->pool,buf,size));
		/* fprintf(stderr,"caught WriteConsoleEx(%x,%d,%d)\n",buf,size,error); */
	} else {
		/* might as well print for debugging */
		fprintf(stderr,"WriteConsoleEx: %s\n",buf);
		/* fprintf(stderr,"NULL Apache request record in WriteConsoleEx()! exiting...\n"); */
		/* exit(-1); */
	}
}

static void WriteConsoleEx_debug(const char *buf, int size, int error){
	fprintf(stderr,"%s: %d %d\n",buf,size,error);
}

/* according to R 2.7.2 the true size of buf is size+1 */
static int ReadConsole(const char *prompt, unsigned char *buf, int size, int addHist){
	apr_size_t len, bpos=0, blen;
	apr_status_t rv;
	SEXP str;
	apr_bucket *bucket;
	const char *data;

	if (!MR_Request.r){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,MR_Request.r,"Can't read with R since MR_Request.r is NULL!");
		return 0;
	}

	if (MR_Request.postParsed){
		RApacheError("Can't read with R since libapreq already started!");
		return 0;
	}

	MR_Request.readStarted = 1;

	if (MR_BBin == NULL){
		MR_BBin = apr_brigade_create(MR_Request.r->pool, MR_Request.r->connection->bucket_alloc);
	}
	rv = ap_get_brigade(MR_Request.r->input_filters, MR_BBin, AP_MODE_READBYTES,
			APR_BLOCK_READ, size);

	if (rv != APR_SUCCESS) {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,MR_Request.r,"ReadConsole() returned an error!");
		return 0;
	}
	for (bucket = APR_BRIGADE_FIRST(MR_BBin); bucket != APR_BRIGADE_SENTINEL(MR_BBin); bucket = APR_BUCKET_NEXT(bucket)) {
		if (APR_BUCKET_IS_EOS(bucket)) {
			if (bpos == 0) { /* end of stream and no data , so return NULL */
				apr_brigade_cleanup(MR_BBin);
				return 0;
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
	apr_brigade_cleanup(MR_BBin);

	buf[bpos] = '\0';
	return (int)bpos;
}

/*************************************************************************
 *
 * Helper functions
 *
 *************************************************************************/

static RApacheHandler *GetHandlerFromRequest(const request_rec *r){
	RApacheDirective *d =  ap_get_module_config(r->per_dir_config,&R_module);
	RApacheHandler *h;

	if (d == NULL || d->handlerKey == NULL){
		ap_set_content_type((request_rec *)r,"text/html");
		RApacheError(apr_psprintf(r->pool,"No RApache Directive specified for 'SetHandler %s'",r->handler));
		return NULL;
	}
   
	h = (RApacheHandler *)apr_hash_get(
			MR_HandlerCache,d->handlerKey,APR_HASH_KEY_STRING);

	if (!h){
		h = (RApacheHandler *)apr_pcalloc(MR_Pool,sizeof(RApacheHandler));
		apr_hash_set(MR_HandlerCache,d->handlerKey,APR_HASH_KEY_STRING,h);
	}

	if (h == NULL || (d->file == NULL && d->function == NULL)){
		ap_set_content_type((request_rec *)r,"text/html");
		RApacheError(apr_psprintf(r->pool,"Invalid RApache Directive: %s",d->handlerKey));
		return NULL;
	}

	h->directive = d;

	return(h);
}

static int SetUpRequest(const request_rec *r){

	/* Acquire R mutex */
    if (MR_mutex != NULL && apr_thread_mutex_lock(MR_mutex) != APR_SUCCESS) {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"RApache Mutex error!");
		return 0;
	}

	/* Set current request_rec */
	MR_Request.r = (request_rec *)r;

	/* Init reading */
	MR_BBin = NULL;

	/* Set output brigade */
	if ((MR_BBout = apr_brigade_create(r->pool, r->connection->bucket_alloc)) == NULL){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"RApache MR_BBout error!");
		if (MR_mutex != NULL) apr_thread_mutex_unlock(MR_mutex);
	   	return 0;
	}

	return 1;
}

static void EmptyRequest(void){
	apr_brigade_cleanup(MR_BBout);
}

/* No need to free any memory here since it was allocated out of the request pool */
static void TearDownRequest(int flush){
	/* Clean up reading */
	if (MR_BBin){
		if (MR_Request.readStarted) {
			/* TODO: check if this succeeds */
			ExecRCode(".Internal(clearPushBack(stdin()))",R_GlobalEnv,NULL);
		}
		apr_brigade_cleanup(MR_BBin);
		apr_brigade_destroy(MR_BBin);
	}
	MR_BBin = NULL;
	/* Clean up writing */
	if (MR_BBout){

		/* A reason not to flush when the brigade is not empty is to
		 * preserve error conditons
		 */
		if(!APR_BRIGADE_EMPTY(MR_BBout) && flush){
			ap_filter_flush(MR_BBout,MR_Request.r->output_filters);
		}
		apr_brigade_cleanup(MR_BBout);
		apr_brigade_destroy(MR_BBout);
	}
	MR_BBout = NULL;
	bzero(&MR_Request,sizeof(RApacheRequest));
	MR_Request.outputErrors = -1;

    if (MR_mutex != NULL) apr_thread_mutex_unlock(MR_mutex);
}

/* Can be called with NULL msg to force proper request handler return
 * value.
 */
static int RApacheResponseError(char *msg){
	int output;
	if (MR_Request.outputErrors==-1){
		/* Module-wide config */
		output = MR_OutputErrors;
	} else {
		/* Per-request config */
		output = MR_Request.outputErrors;
	}

	if (msg) RApacheError(msg);
	if (output){
		TearDownRequest(1);
		return DONE; 
	} else {
		TearDownRequest(0); /* delete all buffered output */
		return HTTP_INTERNAL_SERVER_ERROR;
	}
}

static void RApacheError(char *msg){
	int output;

	if (!msg) return;

	if (MR_Request.outputErrors==-1){
		/* Module-wide config */
		output = MR_OutputErrors;
	} else {
		/* Per-request config */
		output = MR_Request.outputErrors;
	}

	if (output && MR_BBout){
		char *prefix = (MR_Request.errorPrefix)? MR_Request.errorPrefix : (char *)MR_ErrorPrefix;
		char *suffix = (MR_Request.errorSuffix)? MR_Request.errorSuffix : (char *)MR_ErrorSuffix;
		ap_fputs(MR_Request.r->output_filters,MR_BBout,prefix);
		ap_fputs(MR_Request.r->output_filters,MR_BBout,"RApache Warning/Error!!!<br><br>");
		ap_fputs(MR_Request.r->output_filters,MR_BBout,msg);
		ap_fputs(MR_Request.r->output_filters,MR_BBout,suffix);
	} else {
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,MR_Request.r,msg);
	}
}

static void init_R(apr_pool_t *p){
	char *argv[] = {"mod_R", "--gui=none", "--slave", "--silent", "--vanilla","--no-readline"};
	int argc = sizeof(argv)/sizeof(argv[0]);
	int threaded;
	apr_status_t rv;

	if (MR_InitStatus != 0) return;

	MR_InitStatus = 1;

	InitRApachePool(); /* possibly done already if REvalOnStartup or RSourceOnStartup set */

	/* Setup thread mutex if we're running in a threaded server.*/
	rv = ap_mpm_query(AP_MPMQ_IS_THREADED,&threaded);
	if (rv != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: Can't query the server to dermine if it's threaded!\n");
		exit(-1);
	}

	if (threaded){
		rv = apr_thread_mutex_create(&MR_mutex,APR_THREAD_MUTEX_DEFAULT,MR_Pool);
		if (rv != APR_SUCCESS){
			fprintf(stderr,"Fatal Error: unable to create R mutex!\n");
			exit(-1);
		}
	}

	if (apr_env_set("R_HOME",R_HOME,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not set R_HOME from init!\n");
		exit(-1);
	}

	/* Don't let R set up its own signal handlers */
	R_SignalHandlers = 0;

	InitTempDir(p);

	Rf_initEmbeddedR(argc, argv);

	#ifdef CSTACK_DEFNS
	/* Don't do any stack checking */
	R_CStackLimit = -1;
	#endif

	RegisterCallSymbols();

	InitRApacheEnv();

	InitCGIexprs(); 

	/* Execute all *OnStartup code */
	apr_table_do(OnStartupCallback,NULL,MR_OnStartup,NULL);

	/* For the RFile directive */
	if (!(MR_ParsedFileCache = apr_hash_make(MR_Pool))){
		fprintf(stderr,"Fatal Error: could not apr_hash_make() from MR_Pool!\n");
		exit(-1);
	}

	/* Handler Cache */
	if (!(MR_HandlerCache = apr_hash_make(MR_Pool))){
		fprintf(stderr,"Fatal Error: could not apr_hash_make() from MR_Pool!\n");
		exit(-1);
	}

	/* Initialize libapreq2 */
	apreq_initialize(MR_Pool);

	/* Lastly, now redirect R's inputs and outputs */
	R_Consolefile = NULL;
	R_Outputfile = NULL;
	ptr_R_Suicide = Suicide;
	ptr_R_ShowMessage = ShowMessage;
	ptr_R_WriteConsole = NULL;
	ptr_R_WriteConsoleEx = WriteConsoleEx;
	ptr_R_ReadConsole = ReadConsole;
	ptr_R_ResetConsole = ptr_R_FlushConsole = ptr_R_ClearerrConsole = NoOpConsole;
	ptr_R_Busy = NoOpBusy;
	ptr_R_CleanUp = NoOpCleanUp;
	ptr_R_ShowFiles = NoOpShowFiles;
	ptr_R_ChooseFile = NoOpChooseFile;
	ptr_R_EditFile = NoOpEditFile;
	ptr_R_loadhistory = ptr_R_savehistory = ptr_R_addhistory = NoOpHistoryFun;
}

static int decode_return_value(SEXP ret)
{
	if (IS_INTEGER(ret) && LENGTH(ret) == 1){
		return asInteger(ret);
	}

	return DONE;
}

int call_fun1str( char *funstr, char *argstr){
	SEXP val, expr, fun, arg;
	int errorOccurred;

	/* Call funstr */
	fun = MyfindFun(Rf_install(funstr), R_GlobalEnv);
	PROTECT(fun);

	/* argument */
	PROTECT(arg = NEW_CHARACTER(1));
	SET_STRING_ELT(arg, 0, COPY_TO_USER_STRING(argstr));

	/* expression */
	PROTECT(expr = allocVector(LANGSXP,2));
	SETCAR(expr,fun);
	SETCAR(CDR(expr),arg);

	errorOccurred=1;
	val = R_tryEval(expr,R_GlobalEnv,&errorOccurred);
	UNPROTECT(3);

	return (errorOccurred)? 0:1;
}

void InitRApachePool(void){

	if (MR_Pool) return;

	if (apr_pool_create(&MR_Pool,NULL) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not apr_pool_create(MR_Pool)!\n");
		exit(-1);
	}

	/* Table to hold *OnStartup code. Allocate 8 entries. */
	if (!(MR_OnStartup = apr_table_make(MR_Pool,8))){
		fprintf(stderr,"Fatal Error: could not apr_table_make(MR_Pool)!\n");
		exit(-1);
	}

}

/*
 * This is a bit of magic. Upon startup, Apache parses its config file twice, and
 * we really only want to do useful stuff on the second pass, so we use an apr pool
 * feature called apr_pool_userdata_[set|get]() to store state from 1 config pass
 * to the next.
 */
void init_config_pass(apr_pool_t *p){
	void *cfg_pass = NULL;
	const char *userdata_key = "R_language";

	apr_pool_userdata_get(&cfg_pass, userdata_key, p);
	if (!cfg_pass) {
		cfg_pass = apr_pcalloc(p, sizeof(int));
		*((int *)cfg_pass) = 1;
		apr_pool_userdata_set(cfg_pass, userdata_key, apr_pool_cleanup_null, p);
	} else {
		*((int *)cfg_pass) = 2;
	}
	MR_ConfigPass = *((int *)cfg_pass);
}

/*************************************************************************
 *
 * R specific functions
 *
 *************************************************************************/

void InitTempDir(apr_pool_t *p)
{
	const char *tmp;

	if (apr_temp_dir_get(&tmp,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: apr_temp_dir_get() failed!\n");
		exit(-1);
	}

	R_TempDir=(char *)tmp;

	if (apr_env_set("R_SESSION_TMPDIR",R_TempDir,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not set up R_SESSION_TMPDIR!\n");
		exit(-1);
	}
}


static SEXP NewLogical(int tf) {
	SEXP stf;
	PROTECT(stf = NEW_LOGICAL(1));
	LOGICAL_DATA(stf)[0] = tf;
	UNPROTECT(1);
	return stf;
}

static SEXP NewInteger(int i){
	SEXP val;
	PROTECT(val = NEW_INTEGER(1));
	INTEGER_DATA(val)[0] = i;
	UNPROTECT(1);
	return val;
}

static SEXP NewEnv(SEXP enclos){
	SEXP env;
	PROTECT(env = allocSExp(ENVSXP));

	SET_FRAME(env, R_NilValue);
	SET_ENCLOS(env, (enclos)? enclos: R_GlobalEnv);
	SET_HASHTAB(env, R_NilValue);
	SET_ATTRIB(env, R_NilValue);

	UNPROTECT(1);

	return env;
}

static int ExecRCode(const char *code, SEXP env, int *error){
	ParseStatus status;
	SEXP cmd, expr, fun;
	int i, errorOccurred=1, retval = 1;

	PROTECT(cmd = allocVector(STRSXP, 1));
	SET_STRING_ELT(cmd, 0, mkChar(code));

	/* fprintf(stderr,"ExecRCode(%s)\n",code); */
	PROTECT(expr = R_ParseVector(cmd, -1, &status,R_NilValue));

	switch (status){
		case PARSE_OK:
			EvalExprs(expr,env,&errorOccurred);
			if (error) *error = errorOccurred;
			if (errorOccurred) retval=0;
		break;
		case PARSE_INCOMPLETE:
		case PARSE_NULL:
		case PARSE_ERROR:
		case PARSE_EOF:
		default:
			retval=0;
		break;
	}
	UNPROTECT(2);

	return retval;
}

static SEXP ExecFun1Arg(SEXP fun, SEXP arg){
	SEXP val, expr;
	int errorOccurred;

	PROTECT(fun); PROTECT(arg);
	PROTECT(expr = allocVector(LANGSXP,2));
	SETCAR(expr,fun);
	SETCAR(CDR(expr),arg);

	errorOccurred=1;
	val = R_tryEval(expr,R_GlobalEnv,&errorOccurred);
	UNPROTECT(3);

	return val;
}

static void StartRprof(){
	SEXP fun, expr;
	int errorOccurred;
	PROTECT(fun = MyfindFun(install("Rprof"),R_GlobalEnv));
	PROTECT(expr = allocVector(LANGSXP,1));
	SETCAR(expr,fun);

	errorOccurred=1;
	R_tryEval(expr,R_GlobalEnv,&errorOccurred);
	UNPROTECT(2);
}

static void StopRprof(){
	SEXP fun, expr ;
	int errorOccurred;
	PROTECT(fun = MyfindFun(install("Rprof"),R_GlobalEnv));
	PROTECT(expr = allocVector(LANGSXP,2));
	SETCAR(expr,fun);
	SETCAR(CDR(expr),R_NilValue);

	errorOccurred=1;
	R_tryEval(expr,R_GlobalEnv,&errorOccurred);
	UNPROTECT(2);
}

static SEXP ParseFile(const char *file, apr_pool_t *pool, apr_size_t fsize, ParseStatus *status){
	apr_file_t *fd;
	apr_size_t bufsize;
	void *buf;
	apr_finfo_t finfo;
	SEXP cmd, expr, srcfile;

	if (apr_file_open(&fd,file,APR_READ,APR_OS_DEFAULT,pool) != APR_SUCCESS)
		return NULL;

	if (!fsize){
		apr_stat(&finfo,file,APR_FINFO_SIZE,pool);
		fsize = finfo.size;
	}

	bufsize = fsize;
	buf = (void *)apr_pcalloc(pool,fsize+1);

	apr_file_read(fd,buf,&bufsize);

	PROTECT(cmd = allocVector(STRSXP, 1));
	SET_STRING_ELT(cmd, 0, mkChar(buf));

	PROTECT(srcfile = NEW_STRING(1));
	SET_STRING_ELT(srcfile, 0, mkChar(file));

	expr = R_ParseVector(cmd,-1,status,srcfile);
	UNPROTECT(2);

	if (*status == PARSE_OK) return expr;

	return NULL;
}

static int PrepareFileExprs(RApacheHandler *h, const request_rec *r, int *fileParsed){
	RApacheParsedFile *parsedFile;
	ParseStatus status;
	apr_finfo_t finfo;
	if (apr_stat(&finfo,h->directive->file,APR_FINFO_MTIME|APR_FINFO_SIZE,r->pool) != APR_SUCCESS){
		RApacheResponseError(apr_psprintf(r->pool,
					"From RApache Directive: %s\n\tInvalid file: %s\n",
					h->directive->handlerKey,h->directive->file));
		return 0;
	}
	if (h->parsedFile) parsedFile = h->parsedFile;
	else {
		/* Grab file cache element */
		parsedFile = (RApacheParsedFile *)apr_hash_get(MR_ParsedFileCache,h->directive->file,APR_HASH_KEY_STRING);

		/* File not cached yet */
		if (!parsedFile){
			parsedFile = (RApacheParsedFile *)apr_pcalloc(MR_Pool,sizeof(RApacheParsedFile));
			apr_hash_set(MR_ParsedFileCache,h->directive->file,APR_HASH_KEY_STRING,parsedFile);
		}
		h->parsedFile = parsedFile;
	}

	/* Parse file */
	if (parsedFile->mtime < finfo.mtime){

		if (*fileParsed) *fileParsed = 1;

		/* Release old parse file expressions for gc */
		if (parsedFile->exprs) R_ReleaseObject(parsedFile->exprs);

		if ((parsedFile->exprs = ParseFile(h->directive->file,r->pool,finfo.size,&status)) == NULL){
			RApacheError(apr_psprintf(r->pool,"ParsedFile(%s) failed!",h->directive->file));
			return 0;
		} else {
			R_PreserveObject(parsedFile->exprs);
			parsedFile->mtime = finfo.mtime;
		}

	} else {
		if (*fileParsed) *fileParsed = 0;
	}
	return 1;
}

static int PrepareHandlerExpr(RApacheHandler *h, const request_rec *r, int handlerType){
	SEXP fun, tmpenv;
	int veclen = (handlerType == R_SCRIPT)? 3 : 1;
	PROTECT(fun);
	PROTECT(h->envir);
	if (h->directive->file){
		fun = MyfindFun(install(h->directive->function),h->envir);
		if (fun ==  R_UnboundValue){
			UNPROTECT(2);
			RApacheError(apr_psprintf(r->pool,"Handler %s not found!",h->directive->function));
			return 0;
		}
	} else {
		if (h->directive->package)
			fun = MyfindFunInPackage(install(h->directive->function),h->directive->package);
		else
			fun = MyfindFun(install(h->directive->function),R_GlobalEnv);
		if (fun ==  R_UnboundValue){
			UNPROTECT(2);
			RApacheError(apr_psprintf(r->pool,"Handler %s not found!",h->directive->function));
			return 0;
		}

		/* Now fix up the environment chain like so:
		 * 1. h->envir contains CGI vars
		 * 2. MR_RApacheEnv contains RApache functions/variables
		 * 3. CLOENV(fun) is original enclosure.
		 */
		tmpenv = CLOENV(fun);
		SET_CLOENV(fun,h->envir);
		SET_ENCLOS(MR_RApacheEnv,tmpenv);
	}

	if (h->expr) R_ReleaseObject(h->expr);
	R_PreserveObject(h->expr = allocVector(LANGSXP,veclen));
	SETCAR(h->expr,fun);

	if (handlerType == R_SCRIPT){
		/* the call will be: fun(file=file,envir=envir) */
		SETCAR(CDR(h->expr),mkString(r->filename));
		SET_TAG(CDR(h->expr),install("file"));
		SETCAR(CDR(CDR(h->expr)),h->envir);
		SET_TAG(CDR(CDR(h->expr)),install("envir"));
	}
	UNPROTECT(2);

	return 1;
}

static SEXP EvalExprs(SEXP exprs, SEXP env, int *error){
	SEXP lastval;
	int i, errorOccurred=1;
	for(i = 0; i < length(exprs); i++){

		/* swap out console writer for debugging */
		/* ptr_R_WriteConsoleEx = WriteConsoleEx_debug;
		Rf_PrintValue(VECTOR_ELT(exprs,i));
		ptr_R_WriteConsoleEx = WriteConsoleEx; */

		lastval = R_tryEval(VECTOR_ELT(exprs, i),env,&errorOccurred);
		if (error) *error = errorOccurred;
		if (errorOccurred) break;
	}
	return(lastval);
}

static void InitRApacheEnv(){
	ParseStatus status;
	SEXP cmd, expr, fun;
	int i, error=1;

	R_PreserveObject(MR_RApacheEnv = NewEnv(R_GlobalEnv));
	ExecRCode(MR_RApacheSource,MR_RApacheEnv,&error);
	if (error){
		fprintf(stderr,"Error eval'ing MR_RApacheSource!\n\n");
		exit(-1);
	}

	defineVar(install("DONE"),NewInteger(-2),MR_RApacheEnv);
	defineVar(install("DECLINED"),NewInteger(-1),MR_RApacheEnv);
	defineVar(install("OK"),NewInteger(0),MR_RApacheEnv);
	defineVar(install("HTTP_CONTINUE"),NewInteger(100),MR_RApacheEnv);
	defineVar(install("HTTP_SWITCHING_PROTOCOLS"),NewInteger(101),MR_RApacheEnv);
	defineVar(install("HTTP_PROCESSING"),NewInteger(102),MR_RApacheEnv);
	defineVar(install("HTTP_OK"),NewInteger(200),MR_RApacheEnv);
	defineVar(install("HTTP_CREATED"),NewInteger(201),MR_RApacheEnv);
	defineVar(install("HTTP_ACCEPTED"),NewInteger(202),MR_RApacheEnv);
	defineVar(install("HTTP_NON_AUTHORITATIVE"),NewInteger(203),MR_RApacheEnv);
	defineVar(install("HTTP_NO_CONTENT"),NewInteger(204),MR_RApacheEnv);
	defineVar(install("HTTP_RESET_CONTENT"),NewInteger(205),MR_RApacheEnv);
	defineVar(install("HTTP_PARTIAL_CONTENT"),NewInteger(206),MR_RApacheEnv);
	defineVar(install("HTTP_MULTI_STATUS"),NewInteger(207),MR_RApacheEnv);
	defineVar(install("HTTP_MULTIPLE_CHOICES"),NewInteger(300),MR_RApacheEnv);
	defineVar(install("HTTP_MOVED_PERMANENTLY"),NewInteger(301),MR_RApacheEnv);
	defineVar(install("HTTP_MOVED_TEMPORARILY"),NewInteger(302),MR_RApacheEnv);
	defineVar(install("HTTP_SEE_OTHER"),NewInteger(303),MR_RApacheEnv);
	defineVar(install("HTTP_NOT_MODIFIED"),NewInteger(304),MR_RApacheEnv);
	defineVar(install("HTTP_USE_PROXY"),NewInteger(305),MR_RApacheEnv);
	defineVar(install("HTTP_TEMPORARY_REDIRECT"),NewInteger(307),MR_RApacheEnv);
	defineVar(install("HTTP_BAD_REQUEST"),NewInteger(400),MR_RApacheEnv);
	defineVar(install("HTTP_UNAUTHORIZED"),NewInteger(401),MR_RApacheEnv);
	defineVar(install("HTTP_PAYMENT_REQUIRED"),NewInteger(402),MR_RApacheEnv);
	defineVar(install("HTTP_FORBIDDEN"),NewInteger(403),MR_RApacheEnv);
	defineVar(install("HTTP_NOT_FOUND"),NewInteger(404),MR_RApacheEnv);
	defineVar(install("HTTP_METHOD_NOT_ALLOWED"),NewInteger(405),MR_RApacheEnv);
	defineVar(install("HTTP_NOT_ACCEPTABLE"),NewInteger(406),MR_RApacheEnv);
	defineVar(install("HTTP_PROXY_AUTHENTICATION_REQUIRED"),NewInteger(407),MR_RApacheEnv);
	defineVar(install("HTTP_REQUEST_TIME_OUT"),NewInteger(408),MR_RApacheEnv);
	defineVar(install("HTTP_CONFLICT"),NewInteger(409),MR_RApacheEnv);
	defineVar(install("HTTP_GONE"),NewInteger(410),MR_RApacheEnv);
	defineVar(install("HTTP_LENGTH_REQUIRED"),NewInteger(411),MR_RApacheEnv);
	defineVar(install("HTTP_PRECONDITION_FAILED"),NewInteger(412),MR_RApacheEnv);
	defineVar(install("HTTP_REQUEST_ENTITY_TOO_LARGE"),NewInteger(413),MR_RApacheEnv);
	defineVar(install("HTTP_REQUEST_URI_TOO_LARGE"),NewInteger(414),MR_RApacheEnv);
	defineVar(install("HTTP_UNSUPPORTED_MEDIA_TYPE"),NewInteger(415),MR_RApacheEnv);
	defineVar(install("HTTP_RANGE_NOT_SATISFIABLE"),NewInteger(416),MR_RApacheEnv);
	defineVar(install("HTTP_EXPECTATION_FAILED"),NewInteger(417),MR_RApacheEnv);
	defineVar(install("HTTP_UNPROCESSABLE_ENTITY"),NewInteger(422),MR_RApacheEnv);
	defineVar(install("HTTP_LOCKED"),NewInteger(423),MR_RApacheEnv);
	defineVar(install("HTTP_FAILED_DEPENDENCY"),NewInteger(424),MR_RApacheEnv);
	defineVar(install("HTTP_UPGRADE_REQUIRED"),NewInteger(426),MR_RApacheEnv);
	defineVar(install("HTTP_INTERNAL_SERVER_ERROR"),NewInteger(500),MR_RApacheEnv);
	defineVar(install("HTTP_NOT_IMPLEMENTED"),NewInteger(501),MR_RApacheEnv);
	defineVar(install("HTTP_BAD_GATEWAY"),NewInteger(502),MR_RApacheEnv);
	defineVar(install("HTTP_SERVICE_UNAVAILABLE"),NewInteger(503),MR_RApacheEnv);
	defineVar(install("HTTP_GATEWAY_TIME_OUT"),NewInteger(504),MR_RApacheEnv);
	defineVar(install("HTTP_VERSION_NOT_SUPPORTED"),NewInteger(505),MR_RApacheEnv);
	defineVar(install("HTTP_VARIANT_ALSO_VARIES"),NewInteger(506),MR_RApacheEnv);
	defineVar(install("HTTP_INSUFFICIENT_STORAGE"),NewInteger(507),MR_RApacheEnv);
	defineVar(install("HTTP_NOT_EXTENDED"),NewInteger(510),MR_RApacheEnv);

	R_LockEnvironment(MR_RApacheEnv, TRUE);

	/* For debugging */
	/* defineVar(install("RApacheEnv"),MR_RApacheEnv,R_GlobalEnv); */
	/* ExecRCode("gctorture()",R_GlobalEnv,&error); */
}

static int OnStartupCallback(void *rec, const char *key, const char *value){
	SEXP e, val;
	ParseStatus status;
	int error=1;

	if (strcmp(key,"eval")==0){
		ExecRCode(value,R_GlobalEnv,&error);
		if (error){
			fprintf(stderr,"\nError evaluating '%s'! Exiting.\n",value);
			exit(-1);
		};
	} else if (strcmp(key,"file")==0){
		e = ParseFile(value,MR_Pool,0,&status);
		if (!e){
			fprintf(stderr,"\nError parsing %s! Exiting.\n",value);
			exit(-1);
		}
		PROTECT(e);
		EvalExprs(e,R_GlobalEnv,&error);
		UNPROTECT(1);
		if (error){
			fprintf(stderr,"\nError evaluating %s! Exiting.\n",value);
			exit(-1);
		}
	} else if (strcmp(key,"package")==0){
		PROTECT(e = allocVector(LANGSXP,6));
		/* library() */
		SETCAR(e,findFun(install("require"),R_GlobalEnv));
		/* package */
		SETCAR(CDR(e),mkString(value));
		/* character.only=TRUE */
		SETCAR(CDR(CDR(e)),NewLogical(TRUE));
		SET_TAG(CDR(CDR(e)),install("character.only"));
		/* logical.return=TRUE */
		SETCAR(CDR(CDR(CDR(e))),NewLogical(TRUE));
		SET_TAG(CDR(CDR(CDR(e))),install("quietly"));
		/* warn.conflicts=FALSE */
		SETCAR(CDR(CDR(CDR(CDR(e)))),NewLogical(FALSE));
		SET_TAG(CDR(CDR(CDR(CDR(e)))),install("warn.conflicts"));
		/* keep.source=FALSE */
		SETCAR(CDR(CDR(CDR(CDR(CDR(e))))),NewLogical(FALSE));
		SET_TAG(CDR(CDR(CDR(CDR(CDR(e))))),install("keep.source"));

		val = R_tryEval(e,R_GlobalEnv, &error);
		UNPROTECT(1);
		if (error){
			fprintf(stderr,"\nError loading package %s! Exiting.",value);
			exit(-1);
		}
	}

	return 1;
}

/* This one doesn't longjmp when function not found */
static SEXP MyfindFun(SEXP symb, SEXP envir){
	SEXP fun;
	SEXPTYPE t;
	fun = findVar(symb,envir);
	t = TYPEOF(fun);

	/* eval promise if need be */
	if (t == PROMSXP){
		int error=1;
		fun = R_tryEval(fun,envir,&error);
		if (error) return R_UnboundValue;
		t = TYPEOF(fun);
	}

	if (t == CLOSXP || t == BUILTINSXP  || t == SPECIALSXP)
		return fun;
	return R_UnboundValue;
}

static SEXP MyfindFunInPackage(SEXP symb, char *package){
	SEXP rho, name, fun;
	SEXPTYPE t;
	SEXP nameSymbol = install("name");
	char *nameptr;
	for (rho = ENCLOS(R_GlobalEnv); rho != R_EmptyEnv; rho = ENCLOS(rho)){
		if (rho == R_BaseEnv) return R_UnboundValue;
		name = getAttrib(rho,nameSymbol);
		if ((nameptr = ap_strchr(CHAR(STRING_ELT(name,0)),':'))){
			if (strcmp(package,nameptr+1)==0){
				fun = findVarInFrame3(rho,symb,TRUE);
				t = TYPEOF(fun);
				/* eval promise if need be */
				if (t == PROMSXP){
					int error=1;
					fun = R_tryEval(fun,rho,&error);
					if (error) return R_UnboundValue;
					t = TYPEOF(fun);
				}

				if (t == CLOSXP || t == BUILTINSXP || t == SPECIALSXP)
					return fun;
			}
		}
	}
	return R_UnboundValue;
}

#define PUTS(s) ap_fputs(MR_Request.r->output_filters,MR_BBout,s)
#define EXEC(s) ExecRCode(s,envir,&errorOccurred); if (errorOccurred) return RApacheResponseError(NULL)
static int RApacheInfo()
{
	SEXP envir = NewEnv(MR_RApacheEnv);
	int errorOccurred=1;
	ap_set_content_type( MR_Request.r, "text/html");

EXEC("hrefify <- function(title) gsub('[\\\\.()]','_',title,perl=TRUE)");
EXEC("cl<-'e'");
EXEC("scrub <- function(st){ if (is.null(st)) return('null'); if (is.na(st)) return('NA'); if (length(st) == 0) return ('length 0 sting'); if (typeof(st) == 'closure') { sink(textConnection('stt','w')); str(st); sink(); st <- stt; } else {st  <- as.character(st) } ; st <- gsub('&','&amp;',st); st <- gsub('@','_at_',st); st <- gsub('<','&lt;',st); st <- gsub('>','&gt;',st); if (length(st) == 0 || is.null(st) || st == '') st <- '&nbsp;'; st }");
EXEC("zebelem <- function(n,v) { cl <<- ifelse(cl=='e','o','e'); cat('<tr class=\"',cl,'\">'); if(!is.na(n)) cat('<td class=\"l\">',n,'</td>'); cat('<td>'); if (length(v)>1) zebra(NULL,v) else cat(scrub(v)); cat('</td></tr>\n'); }");
EXEC("zebra <- function(title,l){ if (!is.null(title)) cat('<h2><a name=\"',hrefify(title),'\"> </a>',title,'</h2>',sep=''); cat('<table><tbody>',sep=''); n <- names(l); mapply(zebelem,if(is.null(n)) rep(NA,length(l)) else n, l); cat('</tbody></table>\n') }");
EXEC(" zebrifyPackage <-function(package){ zebra(package,unclass(packageDescription(package))); cat('<br/><hr/>\\n') }");

/* Header */
PUTS(DOCTYPE_XHTML_1_0T);
PUTS("<html><head>");
PUTS("<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />");
PUTS("<style type=\"text/css\">");
PUTS("body { font-family: \"lucida grande\",verdana,sans-serif; margin-left: 210px; margin-right: 18px; }");
PUTS("table { border: 1px solid #8897be; border-spacing: 0px; font-size: 10pt; }");
PUTS("td { border-bottom:1px solid #d9d9d9; border-left:1px solid #d9d9d9; border-spacing: 0px; padding: 3px 8px; }");
PUTS("td.l { font-weight: bold; width: 10%; }");
PUTS("tr.e { background-color: #eeeeee; border-spacing: 0px; }");
PUTS("tr.o { background-color: #ffffff; border-spacing: 0px; }");
PUTS("div a { text-decoration: none; color: white; }");
PUTS("a:hover { color: #8897be; background: white; }");
PUTS("tr:hover { background: #8897be; }");
PUTS("img.map { position: fixed; border: 0px; left: 50px; right: auto; top: 10px; }");
PUTS("div.map { background: #8897be; font-weight: bold; color: white; position: fixed; bottom: 30px; height: auto; left: 15px; right: auto; top: 110px; width: 150px; padding: 0 13px; text-align: right; font-size: 12pt; }");
PUTS("div.map p { font-size: 10pt; font-family: serif; font-style: italic; }");
PUTS("div.h { font-size: 20pt; font-weight: bold; }");
PUTS("h4 { font-size: 10pt; font-weight: bold; color: grey;}");
PUTS("hr {background-color: #cccccc; border: 0px; height: 1px; color: #000000;}");
PUTS("</style>");
PUTS("<title>RApacheInfo()</title>");
PUTS("<meta name=\"ROBOTS\" content=\"NOINDEX,NOFOLLOW,NOARCHIVE\" />");
PUTS("</head>");
PUTS("<body>");
PUTS("<a name=\"Top\"> </a>");
PUTS("<a href=\"http://www.r-project.org/\"><img class=\"map\" alt=\"R Language Home Page\" src=\"http://www.r-project.org/Rlogo.jpg\"/></a>");

/* RApache version info */
PUTS("<div class=\"h\">RApache version "); PUTS(MOD_R_VERSION); PUTS("<h4>"); PUTS(SVNID); PUTS("</h4></div>");

PUTS("<div class=\"map\">");
PUTS("<p>jump to:</p>");
PUTS("<a href=\"#Top\">Top</a><br/><hr/>");

PUTS("<a href=\"#"); EXEC("cat(hrefify('R.version'))"); PUTS("\">R.version</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('search()'))"); PUTS("\">search()</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('.libPaths()'))"); PUTS("\">.libPaths()</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('options()'))"); PUTS("\">options()</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('Sys.getenv()'))"); PUTS("\">Sys.getenv()</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('Sys.info()'))"); PUTS("\">Sys.info()</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('.Machine'))"); PUTS("\">.Machine</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('.Platform'))"); PUTS("\">.Platform</a><br/>");
PUTS("<a href=\"#"); EXEC("cat(hrefify('Cstack_info()'))"); PUTS("\">Cstack_info()</a><br/><hr/>");

PUTS("<a href=\"#Attached_Packages\">Attached Packages</a><br/><hr/>");
PUTS("<a href=\"#Installed_Packages\">Installed Packages</a><br/><hr/>");
PUTS("<a href=\"#License\">License</a><br/><hr/>");
PUTS("<a href=\"#People\">People</a>");
PUTS("</div>");

EXEC("zebra('R.version',R.version)"); PUTS("<br/><hr/>");
EXEC("zebra('search()',search())"); PUTS("<br/><hr/>");
EXEC("zebra('.libPaths()',.libPaths())"); PUTS("<br/><hr/>");
EXEC("zebra('options()',options())"); PUTS("<br/><hr/>");
EXEC("zebra('Sys.getenv()',as.list(Sys.getenv()))"); PUTS("<br/><hr/>");
EXEC("zebra('Sys.info()',as.list(Sys.info()))"); PUTS("<br/><hr/>");
EXEC("zebra('.Machine',.Machine)"); PUTS("<br/><hr/>");
EXEC("zebra('.Platform',.Platform)"); PUTS("<br/><hr/>");
EXEC("zebra('Cstack_info()',as.list(Cstack_info()))"); PUTS("<br/><hr/>");

PUTS("<h1><a name=\"Attached_Packages\"></a>Attached Packages</h1>");
EXEC("lapply(sub('package:','',search()[grep('package:',search())]),zebrifyPackage)");
PUTS("<h1><a name=\"Installed_Packages\"></a>Installed Packages</h1>");
EXEC("lapply(attr(installed.packages(),'dimnames')[[1]],zebrifyPackage)");

/* Footer */
PUTS("<h2><a name=\"License\"></a>License</h2>");
PUTS("<pre>");
PUTS("Copyright 2005  The Apache Software Foundation\n");
PUTS("\n");
PUTS("Licensed under the Apache License, Version 2.0 (the \"License\");\n");
PUTS("you may not use this file except in compliance with the License.\n");
PUTS("You may obtain a copy of the License at\n");
PUTS("\n");
PUTS("  <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">http://www.apache.org/licenses/LICENSE-2.0\"</a>\n");
PUTS("\n");
PUTS("Unless required by applicable law or agreed to in writing, software\n");
PUTS("distributed under the License is distributed on an \"AS IS\" BASIS,\n");
PUTS("WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n");
PUTS("See the License for the specific language governing permissions and\n");
PUTS("limitations under the License.\n");
PUTS("</pre><hr/>\n");
PUTS("<h2><a name=\"People\"></a>People</h2>\n");
PUTS("<p>Thanks to the following people for their contributions, giving advice, noticing when things were broken and such. If I've forgotten to mention you, please email me.</p>\n");
PUTS("<pre>\n");
PUTS("	Gregoire Thomas\n");
PUTS("	Jan de Leeuw\n");
PUTS("	Keven E. Thorpe\n");
PUTS("	Jeremy Stephens\n");
PUTS("	Aleksander Wawer\n");
PUTS("	David Konerding\n");
PUTS("	Robert Kofler\n");
PUTS("	Jeroen Ooms\n");
PUTS("</pre>");
PUTS("</body></html>");

return DONE;
}
#undef PUTS
#undef EXEC

struct TableCtx {
	SEXP list;
	SEXP names;
	int i;
};

static int TableCallback(void *datum,const char *n, const char *v){
	struct TableCtx *ctx = (struct TableCtx *)datum;
	SEXP value;

	if (!v || !strcmp(v,"")){
		value = R_NilValue;
	} else {
		value = NEW_CHARACTER(1);
		STRING_PTR(value)[0] = mkChar(v);
	}

	STRING_PTR(ctx->names)[ctx->i]=mkChar(n);
	SET_ELEMENT(ctx->list,ctx->i,value);
	ctx->i += 1;
}

static SEXP AprTableToList(apr_table_t *t){
	int len; 
	struct TableCtx ctx;

	if (!t) return R_NilValue;
	len = apr_table_elts(t)->nelts;
	if (!len) return R_NilValue;

	PROTECT(ctx.list = NEW_LIST(len));
	PROTECT(ctx.names = NEW_STRING(len));
	ctx.i = 0;
	apr_table_do(TableCallback,(void *)&ctx,t,NULL);
	SET_NAMES(ctx.list,ctx.names);
	UNPROTECT(2);
	return ctx.list;
}

static void InitCGIexprs(){
	int i, numVars=0;
	SEXP symdA, symdC, expdA, expdC, str;

	PROTECT(symdA = findFun(install("delayedAssign"),R_GlobalEnv));
	PROTECT(symdC = findFun(install(".Call"),R_GlobalEnv));

	for (i = 0; MR_CGIvars[i] ; i+=2){
		numVars++;
	}

	PROTECT(MR_CGIexprs = allocVector(EXPRSXP,numVars));
	numVars = 0;
	for (i = 0; MR_CGIvars[i] ; i+=2){

		/* create .Call('bar') expr */
		PROTECT(str = NEW_STRING(1));
		CHARACTER_DATA(str)[0] = mkChar(MR_CGIvars[i+1]);
		PROTECT(expdC = allocVector(LANGSXP,2));
		SETCAR(expdC,symdC);
		SETCAR(CDR(expdC),str);

		/* create delayedAssign('foo',.Call('bar'),NULL,NULL); */
		PROTECT(str = NEW_STRING(1));
		CHARACTER_DATA(str)[0] = mkChar(MR_CGIvars[i]);
		PROTECT(expdA = allocVector(LANGSXP,5));
		SETCAR(expdA,symdA);
		SETCAR(CDR(expdA),str);
		SETCAR(CDR(CDR(expdA)), expdC);
		SETCAR(CDR(CDR(CDR(expdA))), R_NilValue);
		SETCAR(CDR(CDR(CDR(CDR(expdA)))), R_NilValue);

		SET_VECTOR_ELT(MR_CGIexprs,numVars++,expdA);
	}
	UNPROTECT(3 + (4 * numVars));
	R_PreserveObject(MR_CGIexprs);
}

static void InjectCGIvars(SEXP env){
	int i, error=1,len;
	SEXP expr;

	PROTECT(env);
	len = LENGTH(MR_CGIexprs);
	for (i = 0; i < len; i++){
		expr = VECTOR_ELT(MR_CGIexprs,i);
		/* set the eval.env and assign.env args to  delayedAssign. */
		SETCAR(CDR(CDR(CDR(expr))), env);
		SETCAR(CDR(CDR(CDR(CDR(expr)))), env);
	}
	EvalExprs(MR_CGIexprs,env,&error);
	UNPROTECT(1);
	if (error){
		fprintf(stderr,"Could not inject CGI VARS!!!!\n");
		return;
	}
}

/*************************************************************************
 *
 * .Call functions: used from R code.
 *
 *************************************************************************/

SEXP RApache_setHeader(SEXP header, SEXP value){
	const char *key = CHAR(STRING_PTR(header)[0]);
	const char *val;

	if (!key) return NewLogical(FALSE);
   
	if (value == R_NilValue){
		apr_table_unset(MR_Request.r->headers_out,key);
	} else {
		val = CHAR(STRING_PTR(value)[0]);
		if (!val) return NewLogical(FALSE);
		apr_table_set(MR_Request.r->headers_out,key,val);
	}

	return NewLogical(TRUE);
}

SEXP RApache_setContentType(SEXP stype){
	const char *ctype;
	if (stype == R_NilValue) return NewLogical(FALSE);
	ctype = CHAR(STRING_PTR(stype)[0]);
	if (!ctype) return NewLogical(FALSE);
	ap_set_content_type( MR_Request.r, apr_pstrdup(MR_Request.r->pool,ctype));
	return NewLogical(TRUE);
}

SEXP RApache_setCookie(SEXP sname, SEXP svalue, SEXP sexpires, SEXP spath, SEXP sdomain, SEXP therest){
	const char *name, *value, *cookie;
	char strExpires[APR_RFC822_DATE_LEN];
	apr_time_t texpires;

	/* name */
	if (sname == R_NilValue) return NewLogical(FALSE);
	else name = CHAR(STRING_PTR(sname)[0]);

	/* value */
	if (svalue == R_NilValue || LENGTH(svalue) != 1)
		value = "";
	else {
		svalue = coerceVector(svalue,STRSXP);
		value = (svalue != NA_STRING)? CHAR(STRING_PTR(svalue)[0]): "";
	}

	cookie = apr_pstrcat(MR_Request.r->pool,name,"=",value,NULL);

	/* expires */
	if (sexpires != R_NilValue && inherits(sexpires,"POSIXt") ){
		SEXP tmpExpires;
		if (inherits(sexpires,"POSIXct")){
			tmpExpires = sexpires;
		} else if (inherits(sexpires,"POSIXlt")){
			tmpExpires = ExecFun1Arg(MyfindFun(install("as.POSIXct"),R_GlobalEnv),sexpires);
		}
		apr_time_ansi_put(&texpires,(time_t)(REAL(tmpExpires)[0]));
		apr_rfc822_date(strExpires, texpires);

		cookie = apr_pstrcat(MR_Request.r->pool,cookie,";expires=",strExpires,NULL);
	}

	/* path */
	if (spath != R_NilValue && isString(spath))
		cookie = apr_pstrcat(MR_Request.r->pool,cookie,";path=",CHAR(STRING_PTR(spath)[0]),NULL);
	/* domain */
	if (sdomain != R_NilValue && isString(sdomain))
		cookie = apr_pstrcat(MR_Request.r->pool,cookie,";domain=",CHAR(STRING_PTR(sdomain)[0]),NULL);
	/* therest */
	if (therest != R_NilValue && isString(therest) && CHAR(STRING_PTR(therest)[0])[0] != '\0'){
		fprintf(stderr,"therest is <%s>\n",CHAR(STRING_PTR(therest)[0]));
		cookie = apr_pstrcat(MR_Request.r->pool,cookie,";",CHAR(STRING_PTR(therest)[0]),NULL);
	}

	if (!apr_table_get(MR_Request.r->headers_out,"Cache-Control"))
		apr_table_set(MR_Request.r->headers_out,"Cache-Control","nocache=\"set-cookie\"");

	apr_table_set(MR_Request.r->headers_out,"Set-Cookie",cookie);

	return NewLogical(TRUE);
}

/* HTML Entity en/decoding */
static SEXP encode(const char *str){
	SEXP retstr;
	char *buf;
	int len;

	len = strlen(str);
	buf = R_alloc(3*len+1,sizeof(char));
	if (!buf) return R_NilValue;

	if (apreq_encode(buf,str,len)){
		retstr = mkChar(buf);
	} else {
		retstr = R_NilValue;
	}
	/* free(buf); */

	return retstr;
}

static SEXP decode(const char *str){
	SEXP retstr;
	char *buf;
	apr_size_t len, blen;

	len = strlen(str);
	buf = R_alloc(len,sizeof(char));
	if (!buf) return R_NilValue;

	if (apreq_decode(buf,&blen,str,len) == APR_SUCCESS){
		retstr = mkChar(buf);
	} else {
		retstr = R_NilValue;
	}
	/* free(buf); */

	return retstr;
}

SEXP RApache_urlEnDecode(SEXP str,SEXP enc){
	int vlen, i;
	SEXP new_str;
	SEXP (*endecode)(const char *str);

	if (IS_LOGICAL(enc) && LOGICAL(enc)[0] == TRUE){
		endecode = encode;
	} else {
		endecode = decode;
	}

	if (!IS_CHARACTER(str)){
		warning("RApache_urlEnDecode called with a non-character object!");
		return R_NilValue;
	}
	vlen = LENGTH(str);

	PROTECT(new_str = NEW_STRING(vlen));
	for (i = 0; i < vlen; i++)
		CHARACTER_DATA(new_str)[i] = endecode(CHAR(STRING_PTR(str)[i]));
	UNPROTECT(1);

	return new_str;
}

SEXP RApache_RApacheInfo(){
	RApacheInfo();
	return R_NilValue;
}

SEXP RApache_parseGet() {
	/* If we've already made the table, just hand it out again */
	if (MR_Request.argsTable) return AprTableToList(MR_Request.argsTable);
	/* Don't parse if there aren't an GET variables to parse */
	if(!MR_Request.r->args) return R_NilValue;
	/* First call: parse and return */
	MR_Request.argsTable = apr_table_make(MR_Request.r->pool,APREQ_DEFAULT_NELTS);
	apreq_parse_query_string(MR_Request.r->pool,MR_Request.argsTable,MR_Request.r->args);
	/* TODO: error checking of argsTable here */
	return AprTableToList(MR_Request.argsTable);
}

typedef struct {
	SEXP files;
	SEXP names;
	int i;
} RApacheFileUploads;

static int FileUploadsCallback(void *ft,const char *key, const char *val){
	RApacheFileUploads  *pf = (RApacheFileUploads *)ft;
	apreq_param_t *p = apreq_value_to_param(val);
	apr_file_t *f;
	apr_finfo_t finfo;
	const char *filename;
	SEXP file_elem, name_elem, str1, str2;

	f = apreq_brigade_spoolfile(p->upload);

	/* No file upload */
	if (f == NULL || (apr_file_info_get(&finfo,APR_FINFO_SIZE,f) != APR_SUCCESS) || finfo.size <= 0 ){
		SET_ELEMENT(pf->files,pf->i,R_NilValue);
		STRING_PTR(pf->names)[pf->i]=mkChar(key);
	} else {
		filename = finfo.fname;

		PROTECT(file_elem = NEW_LIST(2));
		PROTECT(name_elem = NEW_STRING(2));
		PROTECT(str1 = NEW_STRING(1));
		PROTECT(str2 = NEW_STRING(1));

		STRING_PTR(str1)[0]=mkChar(val);
		STRING_PTR(str2)[0]=mkChar(filename);

		SET_ELEMENT(file_elem,0,str1);
		SET_ELEMENT(file_elem,1,str2);

		STRING_PTR(name_elem)[0]=mkChar("name");
		STRING_PTR(name_elem)[1]=mkChar("tmp_name");

		SET_NAMES(file_elem,name_elem);

		SET_ELEMENT(pf->files,pf->i,file_elem);
		STRING_PTR(pf->names)[pf->i]=mkChar(key);

		UNPROTECT(4);
	}
		pf->i += 1;

	return 1;
}

static SEXP parsePost(int returnPost) {
	apreq_parser_t *psr;
	apr_bucket_brigade *bb;
	const char *tmpdir;
	apr_status_t s;
	const apr_table_t *uploads;
	SEXP filenames;
	int nfiles;

	if (MR_Request.readStarted) {
		/* If we've already started reading with R then don't try to parse at all. */
		RApacheError("Oops! Your R code has already started reading the request.");
		return R_NilValue;
	} else if (MR_Request.postParsed){
		/* We've already parsed the input, just hand out the result */
		return (returnPost)?AprTableToList(MR_Request.postTable):MR_Request.filesVar;
	}

	/* Don't parse if not a POST */
	if (strcmp(MR_Request.r->method,"POST") != 0) {
		MR_Request.postTable = NULL;
		MR_Request.filesVar = R_NilValue;
		return R_NilValue;
	}

	/* Start parse */
	MR_Request.postParsed=1;

	MR_Request.postTable = apr_table_make(MR_Request.r->pool, APREQ_DEFAULT_NELTS);
	apr_temp_dir_get(&tmpdir,MR_Request.r->pool);
	psr = apreq_parser_make(
			MR_Request.r->pool,
			MR_Request.r->connection->bucket_alloc,
			apr_table_get(MR_Request.r->headers_in,"Content-Type"),
			apreq_parser(apr_table_get(MR_Request.r->headers_in,"Content-Type")),
			0, /* brigade_limit */
			tmpdir,
			NULL,
			NULL);
	bb = apr_brigade_create(MR_Request.r->pool,MR_Request.r->connection->bucket_alloc);
	while((s = ap_get_brigade(MR_Request.r->input_filters,bb,AP_MODE_READBYTES,APR_BLOCK_READ,HUGE_STRING_LEN)) == APR_SUCCESS){ 
		apreq_parser_run(psr,MR_Request.postTable,bb);
		if (APR_BUCKET_IS_EOS(APR_BRIGADE_LAST(bb))) break;
		apr_brigade_cleanup(bb);
	}
	apr_brigade_cleanup(bb);

	/* Now go ahead and set MR_Request.filesVar*/
	uploads = apreq_uploads(MR_Request.postTable,MR_Request.r->pool);
	nfiles = apr_table_elts(uploads)->nelts;

	if (nfiles){
		RApacheFileUploads fu;
		PROTECT(MR_Request.filesVar = NEW_LIST(nfiles));
		PROTECT(filenames = NEW_STRING(nfiles));

		fu.files = MR_Request.filesVar;
		fu.names = filenames;
		fu.i = 0;
		apr_table_do(FileUploadsCallback,(void *)&fu,uploads,NULL);
		SET_NAMES(MR_Request.filesVar,filenames);
		UNPROTECT(2);
	} else {
		MR_Request.filesVar = R_NilValue;
	}

	return (returnPost)?AprTableToList(MR_Request.postTable):MR_Request.filesVar;
}

SEXP RApache_parsePost(){ return parsePost(1); }

SEXP RApache_parseFiles(){ return parsePost(0); }

SEXP RApache_parseCookies(SEXP sreq){
	const char *cookies;

	if (MR_Request.cookiesTable) 
		return AprTableToList(MR_Request.cookiesTable);

	cookies = apr_table_get(MR_Request.r->headers_in, "Cookie");

	if (cookies == NULL) return R_NilValue;

	MR_Request.cookiesTable = apr_table_make(MR_Request.r->pool,APREQ_DEFAULT_NELTS);
	apreq_parse_cookie_header(MR_Request.r->pool,MR_Request.cookiesTable,cookies);

	return AprTableToList(MR_Request.cookiesTable);
}

#define TABMBR(n,v) STRING_PTR(names)[i]=mkChar(n); SET_ELEMENT(MR_Request.serverVar,i++,AprTableToList(v))
#define INTMBR(n,v) STRING_PTR(names)[i]=mkChar(n); val = NEW_INTEGER(1); INTEGER_DATA(val)[0] = v; SET_ELEMENT(MR_Request.serverVar,i++,val)
#define STRMBR(n,v) STRING_PTR(names)[i]=mkChar(n); if (v){ val = NEW_STRING(1); STRING_PTR(val)[0] = mkChar(v);} else { val = R_NilValue;}; SET_ELEMENT(MR_Request.serverVar,i++,val)
#define LGLMBR(n,v) STRING_PTR(names)[i]=mkChar(n); SET_ELEMENT(MR_Request.serverVar,i++,NewLogical(v));
#define OFFMBR(n,v) STRING_PTR(names)[i]=mkChar(n); val = NEW_NUMERIC(1); NUMERIC_DATA(val)[0] = (double)v; SET_ELEMENT(MR_Request.serverVar,i++,val)
#define TIMMBR(n,v) STRING_PTR(names)[i]=mkChar(n); val = NEW_NUMERIC(1); NUMERIC_DATA(val)[0] = (double)apr_time_sec(v); class = NEW_STRING(2); STRING_PTR(class)[0] = mkChar("POSIXt"); STRING_PTR(class)[1] = mkChar("POSIXct"); SET_CLASS(val,class); SET_ELEMENT(MR_Request.serverVar,i++,val)
SEXP RApache_getServer(){
	int len = 31, i = 0;
	SEXP names, val, class;
	if (!MR_Request.r) return R_NilValue;
	if (MR_Request.serverVar) return MR_Request.serverVar;

	PROTECT(MR_Request.serverVar = NEW_LIST(len));
	PROTECT(names = NEW_STRING(len));

	TABMBR("headers_in",MR_Request.r->headers_in);
	INTMBR("proto_num",MR_Request.r->proto_num);
	STRMBR("protocol",MR_Request.r->protocol);
	STRMBR("unparsed_uri",MR_Request.r->unparsed_uri);
	STRMBR("uri",MR_Request.r->uri);
	STRMBR("filename",MR_Request.r->filename);
	STRMBR("canonical_filename",MR_Request.r->canonical_filename);
	STRMBR("path_info",MR_Request.r->path_info);
	STRMBR("args",MR_Request.r->args);
	STRMBR("content_type",MR_Request.r->content_type);
	STRMBR("handler",MR_Request.r->handler);
	STRMBR("content_encoding",MR_Request.r->content_encoding);
	STRMBR("range",MR_Request.r->range);
	STRMBR("hostname",MR_Request.r->hostname);
	STRMBR("user",MR_Request.r->user);
	LGLMBR("header_only",MR_Request.r->header_only);
	LGLMBR("no_cache",MR_Request.r->no_cache);
	LGLMBR("no_local_copy",MR_Request.r->no_local_copy);
	LGLMBR("assbackwards",MR_Request.r->assbackwards);
	INTMBR("status",MR_Request.r->status);
	INTMBR("method_number",MR_Request.r->method_number);
	LGLMBR("eos_sent",MR_Request.r->eos_sent);
	STRMBR("the_request",MR_Request.r->the_request);
	STRMBR("method",MR_Request.r->method);
	STRMBR("status_line",MR_Request.r->status_line);
	OFFMBR("bytes_sent",MR_Request.r->bytes_sent);
	OFFMBR("clength",MR_Request.r->clength);
	OFFMBR("remaining",MR_Request.r->remaining);
	OFFMBR("read_length",MR_Request.r->read_length);
	TIMMBR("request_time",MR_Request.r->request_time);
	TIMMBR("mtime",MR_Request.r->mtime);

	SET_NAMES(MR_Request.serverVar,names);
	UNPROTECT(2);
	return MR_Request.serverVar;
}


static void swapb(void *result, int size)
{
    int i;
    char *p = result, tmp;

    if (size == 1) return;
    for (i = 0; i < size/2; i++) {
	tmp = p[i];
	p[i] = p[size - i - 1];
	p[size - i - 1] = tmp;
    }
}

SEXP RApache_sendBin(SEXP object, SEXP ssize, SEXP sswap){
	SEXP ans = R_NilValue;
	int i, j, size, swap, len, n = 0;
	const char *s;
	char *buf;
	/* Rboolean wasopen = TRUE, isRaw = FALSE;
	Rconnection con = NULL; */

	/* checkArity(op, args); */
	/* object = CAR(args); */
	if(!isVectorAtomic(object))
		error("'x' is not an atomic vector type");

	/*
	if(TYPEOF(CADR(args)) == RAWSXP) {
		isRaw = TRUE;
	} else {
		con = getConnection(asInteger(CADR(args)));
		if(con->text) error(_("can only write to a binary connection"));
		wasopen = con->isopen;
		if(!con->canwrite)
			error(_("cannot write to this connection"));
	} */

	size = asInteger(ssize);
	swap = asLogical(sswap);

	if(swap == NA_LOGICAL)
		error("invalid '%s' argument"), "swap";

	len = LENGTH(object);
	/*
	if(len == 0) {
		if(isRaw) return allocVector(RAWSXP, 0); else return R_NilValue;
	} */
	if (len == 0) return R_NilValue;

	/* if(!wasopen)
		if(!con->open(con)) error(_("cannot open the connection")); */


	if(TYPEOF(object) == STRSXP) {
		/* if(isRaw) {
			Rbyte *bytes;
			int np, outlen;
			for(i = 0, outlen = 0; i < len; i++) 
				outlen += strlen(translateChar(STRING_ELT(object, i))) + 1;
			PROTECT(ans = allocVector(RAWSXP, outlen));
			bytes = RAW(ans);
			for(i = 0, np = 0; i < len; i++) {
				s = translateChar(STRING_ELT(object, i));
				memcpy(bytes+np, s, strlen(s) + 1);
				np +=  strlen(s) + 1;
			}
		} else { */
			for(i = 0; i < len; i++) {
				s = translateChar(STRING_ELT(object, i));
				/* n = con->write(s, sizeof(char), strlen(s) + 1, con); */
				/* XXX: apache write */
				n = ap_fwrite(MR_Request.r->output_filters,MR_BBout,s,strlen(s)+1);
				if(!n) {
					warning("problem writing to connection");
					break;
				}
			}
		/* } */
	} else {
		switch(TYPEOF(object)) {
			case LGLSXP:
			case INTSXP:
				if(size == NA_INTEGER) size = sizeof(int);
				switch (size) {
					case sizeof(signed char):
					case sizeof(short):
					case sizeof(int):
#if SIZEOF_LONG == 8
					case sizeof(long):
#elif SIZEOF_LONG_LONG == 8
					case sizeof(_lli_t):
#endif
						break;
					default:
						error("size %d is unknown on this machine", size);
				}
				break;
			case REALSXP:
				if(size == NA_INTEGER) size = sizeof(double);
				switch (size) {
					case sizeof(double):
					case sizeof(float):
#if SIZEOF_LONG_DOUBLE > SIZEOF_DOUBLE
					case sizeof(long double):
#endif
						break;
					default:
						error("size %d is unknown on this machine", size);
				}
				break;
			case CPLXSXP:
				if(size == NA_INTEGER) size = sizeof(Rcomplex);
				if(size != sizeof(Rcomplex))
					error("size changing is not supported for complex vectors");
				break;
			case RAWSXP:
				if(size == NA_INTEGER) size = 1;
				if(size != 1)
					error("size changing is not supported for raw vectors");
				break;
			default:
				UNIMPLEMENTED_TYPE("writeBin", object);
		}
		buf = R_chk_calloc(len, size); /* R_alloc(len, size); */
		switch(TYPEOF(object)) {
			case LGLSXP:
			case INTSXP:
				switch (size) {
					case sizeof(int):
						memcpy(buf, INTEGER(object), size * len);
						break;
#if SIZEOF_LONG == 8
					case sizeof(long):
						{
							long l1;
							for (i = 0, j = 0; i < len; i++, j += size) {
								l1 = (long) INTEGER(object)[i];
								memcpy(buf + j, &l1, size);
							}
							break;
						}
#elif SIZEOF_LONG_LONG == 8
					case sizeof(_lli_t):
						{
							_lli_t ll1;
							for (i = 0, j = 0; i < len; i++, j += size) {
								ll1 = (_lli_t) INTEGER(object)[i];
								memcpy(buf + j, &ll1, size);
							}
							break;
						}
#endif
					case 2:
						{
							short s1;
							for (i = 0, j = 0; i < len; i++, j += size) {
								s1 = (short) INTEGER(object)[i];
								memcpy(buf + j, &s1, size);
							}
							break;
						}
					case 1:
						for (i = 0; i < len; i++)
							buf[i] = (signed char) INTEGER(object)[i];
						break;
					default:
						error("size %d is unknown on this machine", size);
				}
				break;
			case REALSXP:
				switch (size) {
					case sizeof(double):
						memcpy(buf, REAL(object), size * len);
						break;
					case sizeof(float):
						{
							float f1;
							for (i = 0, j = 0; i < len; i++, j += size) {
								f1 = (float) REAL(object)[i];
								memcpy(buf+j, &f1, size);
							}
							break;
						}
#if SIZEOF_LONG_DOUBLE > SIZEOF_DOUBLE
					case sizeof(long double):
						{
							/* some systems have problems with memcpy from
							   the address of an automatic long double,
							   e.g. ix86/x86_64 Linux with gcc4 */
							static long double ld1;
							for (i = 0, j = 0; i < len; i++, j += size) {
								ld1 = (long double) REAL(object)[i];
								memcpy(buf+j, &ld1, size);
							}
							break;
						}
#endif
					default:
						error("size %d is unknown on this machine", size);
				}
				break;
			case CPLXSXP:
				memcpy(buf, COMPLEX(object), size * len);
				break;
			case RAWSXP:
				memcpy(buf, RAW(object), len); /* size = 1 */
				break;
		}

		if(swap && size > 1) {
			if (TYPEOF(object) == CPLXSXP)
				for(i = 0; i < len; i++) {
					int sz = size/2;
					swapb(buf+sz*2*i, sz);
					swapb(buf+sz*(2*i+1), sz);
				}
			else 
				for(i = 0; i < len; i++) swapb(buf+size*i, size);
		}

		/* write it now */
		/* if(isRaw) {
			PROTECT(ans = allocVector(RAWSXP, size*len));
			memcpy(RAW(ans), buf, size*len);
		} else { */
			/* XXX: apache write */
			/* n = con->write(buf, size, len, con); */
			n = ap_fwrite(MR_Request.r->output_filters,MR_BBout,buf,size*len);
			if(n < len) warning("problem writing to connection");
		/* } */
		Free(buf);
	}

	/* if(!wasopen) con->close(con);
	 if(isRaw) {
		R_Visible = TRUE;
		UNPROTECT(1);
	} else R_Visible = FALSE; */

	return ans;
}

SEXP RApache_outputErrors(SEXP status,SEXP prefix, SEXP suffix){
	if (isLogical(status)){
	   	if ((LOGICAL(status)[0] == TRUE)){
			MR_Request.outputErrors = 1;
		} else {
			MR_Request.outputErrors = 0;
		}
	} else {
		warning("ROutputErrors called with non-logical status!");
	}

	if (isString(prefix)){
		MR_Request.errorPrefix = apr_pstrdup(MR_Request.r->pool,CHAR(STRING_PTR(prefix)[0]));
	} else if (!isNull(prefix)) {
		warning("ROutputErrors called with non-string prefix!");
	}

	if (isString(suffix)){
		MR_Request.errorSuffix = apr_pstrdup(MR_Request.r->pool,CHAR(STRING_PTR(suffix)[0]));
	} else if (!isNull(suffix)) {
		warning("ROutputErrors called with non-string suffix!");
	}

	return R_NilValue;
}

static void RegisterCallSymbols() {
	R_CallMethodDef callMethods[]  = {
	{"RApache_setHeader", (DL_FUNC) &RApache_setHeader, 2},
	{"RApache_setContentType", (DL_FUNC) &RApache_setContentType, 1},
	{"RApache_setCookie",(DL_FUNC) &RApache_setCookie,6},
	{"RApache_urlEnDecode",(DL_FUNC) &RApache_urlEnDecode,2},
	{"RApache_RApacheInfo",(DL_FUNC) &RApache_RApacheInfo,0},
	{"RApache_parseGet",(DL_FUNC) &RApache_parseGet,0},
	{"RApache_parsePost",(DL_FUNC) &RApache_parsePost,0},
	{"RApache_parseFiles",(DL_FUNC) &RApache_parseFiles,0},
	{"RApache_parseCookies",(DL_FUNC) &RApache_parseCookies,0},
	{"RApache_getServer",(DL_FUNC) &RApache_getServer,0},
	{"RApache_sendBin",(DL_FUNC) &RApache_sendBin,3},
	{"RApache_outputErrors",(DL_FUNC) &RApache_outputErrors,3},
	{NULL, NULL, 0}
	};
	R_registerRoutines(R_getEmbeddingDllInfo(),NULL,callMethods,NULL,NULL);
}
