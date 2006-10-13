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
#define MOD_R_VERSION "mod_R/0.1.5"
#include "mod_R.h" 

#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <dlfcn.h>

/*
 * Include the core server components.
 * 
 * DARWIN's dyld.h defines an enum { FALSE TRUE} which conflicts with R
 */
#ifdef DARWIN
#define ENUM_DYLD_BOOL
enum DYLD_BOOL{ DYLD_FALSE, DYLD_TRUE};
#endif
#include "httpd.h"
#include "http_log.h"
#include "http_config.h"
#include "http_protocol.h"
#include "util_filter.h"
#include "apr_strings.h"
#include "apr_env.h"
#include "apr_pools.h"
#include "apr_hash.h"

/*
 * Include the R headers and hook code specific to mod_R
 */
#include <R.h>
#include <Rversion.h>
#include <Rdefines.h>
#define R_INTERFACE_PTRS
#include <Rinterface.h>
#define R_240 132096 /* This is set in Rversion.h */
#if (R_VERSION >= R_240) /* we can delete this code after R 2.4.0 release */
#include <Rembedded.h>
#endif

/*
 * We're hijacking stdin, stdout, and stderr, and since
 * Rconnections aren't fully opened yet, we need some help.
 */
#include "mod_R_Rconn.h"

/*************************************************************************
 *
 * Globals
 *
 *************************************************************************/

/* Override R's setting of temporary directory */
extern char *R_TempDir;

/* Functions exported by RApache which we need to find at RApache load time */
SEXP (*RA_new_request_rec)(request_rec *r);

/* Current request_rec */
request_rec *MR_Request = NULL;

/* Option to turn on errors in output */
static int MR_OutputErrors = 0;
static char *MR_ErrorPrefix = "<pre style=\"background-color:#eee; padding 10px; font-size: 11px\"><code>";
static char *MR_ErrorSuffix = "</code></pre>";

/* Number of times apache has parsed config files; we'll do stuff on second pass */
static int MR_ConfigPass = 1;

/* Don't start R more than once, if the flag is 1 it's already started */
static int MR_InitStatus = 0;

/* Cache child pid on startup. If we fork() we don't want to do certain
 * cleanup steps, especially on cgi fork
 */
static unsigned long MR_pid;

/* Per-child memory for configuration */
static apr_pool_t *MR_Pool = NULL;
static apr_hash_t *MR_Scripts = NULL;

/*
 * Bucket brigades for reading and writing to client
 * exported to RApache
 */
apr_bucket_brigade *MR_BBin;
apr_bucket_brigade *MR_BBout;

/*************************************************************************
 *
 * RApache types
 *
 *************************************************************************/
typedef struct MR_cfg {
	char *file;
	char *handler;
	int reload;
}  MR_cfg;

/*************************************************************************
 *
 * Function declarations
 *
 * MR_* functions are called by apache code base
 * mr_* functions are called from within mod_R
 *
 *************************************************************************/
/* 
 * Exported by libR
 */
#if (R_VERSION < R_240) /* we can delete this code after R 2.4.0 release */
extern int Rf_initEmbeddedR(int argc, char *argv[]);
#endif

/*
 * Module functions
 */
static void *MR_create_dir_cfg(apr_pool_t *p, char *dir);
static void *MR_merge_dir_cfg(apr_pool_t *p, void *parent, void *new);
static void *MR_create_srv_cfg(apr_pool_t *p, server_rec *s);
static void *MR_merge_srv_cfg(apr_pool_t *p, void *parent, void *new);
static void MR_register_hooks (apr_pool_t *p);

/*
 * Command functions
 */
static const char *MR_cmd_Handler(cmd_parms *cmd, void *conf, const char *handler, const char *file, const char *reload);
static const char *MR_cmd_OutputErrors(cmd_parms *cmd, void *mconfig);

/*
 * Hook functions
 */
static int MR_hook_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s);
static void MR_hook_child_init(apr_pool_t *p, server_rec *s);
static int MR_hook_request_handler (request_rec *r);

/*
 * Exit callback
 */
static apr_status_t MR_child_exit(void *data);

/*
 * Helper functions
 */
void mr_init_pool(void);
void mr_init_config_pass(apr_pool_t *p);
static void mr_init(apr_pool_t *);
static int mr_call_fun1str( char *funstr, char *argstr);
static int mr_decode_return_value(SEXP ret);
static int mr_findFun( char *funstr);
static int mr_check_cfg(request_rec *, MR_cfg *);
static MR_cfg *mr_dir_config(const request_rec *r);

/*
 * R specific functions
 */

void mr_InitTempDir(apr_pool_t *p);
int mr_stdout_vfprintf(Rconnection con, const char *format, va_list ap);
int mr_stdout_fflush(Rconnection con);
size_t mr_stdout_write(const void *ptr, size_t size, size_t n, Rconnection con);
int mr_stderr_vfprintf(Rconnection, const char *format, va_list ap);
int mr_stderr_vfprintf_OutputErrors(Rconnection, const char *format, va_list ap);
int mr_stderr_fflush(Rconnection con);
int mr_stdin_fgetc(Rconnection con);
void mr_WriteConsole(char *, int);
void mr_WriteConsole_OutputErrors(char *, int);



/*************************************************************************
 *
 * Command array
 *
 *************************************************************************/

static const command_rec MR_cmds[] = {
	AP_INIT_TAKE3("RHandler", MR_cmd_Handler, NULL, OR_OPTIONS, "R source file and function to handle request."),
	AP_INIT_NO_ARGS("ROutputErrors", MR_cmd_OutputErrors, NULL, OR_OPTIONS, "Option to print error messages in output."),
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
	MR_create_dir_cfg,        /* dir config creater */
	MR_merge_dir_cfg,         /* dir merger --- default is to override */
	MR_create_srv_cfg,        /* server config */
	MR_merge_srv_cfg,         /* merge server config */
	MR_cmds,                  /* table of config file commands */
	MR_register_hooks,        /* register hooks */
};

/*************************************************************************
 *
 * Module functions: called by apache code base
 *
 *************************************************************************/

static void *MR_create_dir_cfg(apr_pool_t *p, char *dir){
	MR_cfg *cfg;

	cfg = (MR_cfg *)apr_pcalloc(p,sizeof(MR_cfg));

	return (void *)cfg;
}

void *MR_merge_dir_cfg(apr_pool_t *pool, void *parent, void *new){
	MR_cfg *c;
	MR_cfg *p = (MR_cfg *)parent;
	MR_cfg *n = (MR_cfg *)new;

	c = (MR_cfg *)apr_pcalloc(pool,sizeof(MR_cfg));

	/* Add parent stuff, even if null */
	c->file =    p->file;
	c->handler = p->handler;
	c->reload =  p->reload;

	/* Now add new config stuff, overriding parent */
	c->file =    n->file;
	c->handler = n->handler;
	c->reload =  n->reload;

	return (void *)c;
}

void *MR_create_srv_cfg(apr_pool_t *p, server_rec *s){
	MR_cfg *c;

	mr_init_config_pass(s->process->pool);

	c = (MR_cfg *)apr_pcalloc(p,sizeof(MR_cfg));

	return (void *)c;
}

void *MR_merge_srv_cfg(apr_pool_t *pool, void *parent, void *new){
	MR_cfg *c;
	MR_cfg *p = (MR_cfg *)parent;
	MR_cfg *n = (MR_cfg *)new;

	c = (MR_cfg *)apr_pcalloc(pool,sizeof(MR_cfg));

	/* Add parent stuff, even if null */
	c->file =    p->file;
	c->handler = p->handler;
	c->reload =  p->reload;

	/* Now add new config stuff, overriding parent */
	c->file =    n->file;
	c->handler = n->handler;
	c->reload =  n->reload;

	return (void *)c;
}


static void MR_register_hooks (apr_pool_t *p)
{
	ap_hook_post_config(MR_hook_post_config, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_child_init(MR_hook_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(MR_hook_request_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/*************************************************************************
 *
 * Command functions: called by apache code base
 *
 *************************************************************************/

static const char *MR_cmd_Handler(cmd_parms *cmd, void *conf, const char *handler, const char *file, const char *reload){
	MR_cfg *c = (MR_cfg *)conf;

	if (MR_ConfigPass == 1){
		apr_finfo_t finfo;
		if (apr_stat(&finfo,file,APR_FINFO_TYPE,cmd->pool) != APR_SUCCESS){
			return apr_psprintf(cmd->pool,"Rsource: %s file not found!",file);
		}
		return NULL;
	}

	c->file = apr_pstrdup(cmd->pool,file);
	c->handler = apr_pstrdup(cmd->pool,handler);
	if (reload[0] == 'Y' || reload[0] == 'y' || reload[0] == 't' || reload[0] =='T' ||
			reload[0] == '1'){
		c->reload = 1;
	} else {
		c->reload  = 0;
	}

	/* Cache modification time of all script files in the MR_Scripts hash.
	 * It's set to zero by default, so that it can be loaded later
	 */
	mr_init_pool();
	apr_hash_set(MR_Scripts,c->file,APR_HASH_KEY_STRING,apr_pcalloc(MR_Pool,sizeof(apr_time_t)));

	return NULL;
}

static const char *MR_cmd_OutputErrors(cmd_parms *cmd, void *mconfig){
	MR_OutputErrors = 1;
	return NULL;
}
/*************************************************************************
 *
 * Hook functions: called by apache code base
 *
 *************************************************************************/

static int MR_hook_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s){
	ap_add_version_component(pconf,MOD_R_VERSION);
	ap_add_version_component(pconf,apr_psprintf(pconf,"R/%s.%s",R_MAJOR,R_MINOR));
	return OK;
}

static void MR_hook_child_init(apr_pool_t *p, server_rec *s){
	MR_pid=(unsigned long)getpid();
	mr_init(p);
	apr_pool_cleanup_register(p, p, MR_child_exit, MR_child_exit);
}

static int MR_hook_request_handler (request_rec *r)
{
	MR_cfg *c;
	Rconnection con;

	SEXP val, rhandler_exp, rhandler;
	int errorOccurred;

	// Only handle if correct handler
	if (strcmp(r->handler,"r-handler") != 0)
		return DECLINED;

	/* Config */
	c = mr_dir_config(r);

	/* Set current request_rec */
	MR_Request = r;

	/* Init reading */
	MR_BBin = NULL;

	/* Set output brigade early */
	MR_BBout = apr_brigade_create(r->pool, r->connection->bucket_alloc);



	/* Apachify stdout */
	con = getConnection(1);
	con->text = FALSE; /* allows us to do binary writes */
	con->vfprintf = mr_stdout_vfprintf;
	con->write = mr_stdout_write;
	con->fflush = mr_stdout_fflush;

	/* Apachify stderr */
	con = getConnection(2);
	con->vfprintf = (MR_OutputErrors)? mr_stderr_vfprintf_OutputErrors: mr_stderr_vfprintf;
	con->fflush = mr_stderr_fflush;
	/*  Some stderr messages go through here */
	ptr_R_WriteConsole = (MR_OutputErrors)? mr_WriteConsole_OutputErrors : mr_WriteConsole;

	/* 
	 * Bad apache config? 
	 *
	 * Here, we find the handler we must run and
	 * load any scripts or libraries.
	 */
	if (!mr_check_cfg(r,c)){
		if (MR_BBout){
			if(!APR_BRIGADE_EMPTY(MR_BBout)){
				ap_filter_flush(MR_BBout,r->output_filters);
			}
			apr_brigade_cleanup(MR_BBout);
			apr_brigade_destroy(MR_BBout);
		}
		return (MR_OutputErrors)? DONE: DECLINED;
	}



	/*
	 * Now call (c->handler)(req)
	 */

	rhandler = Rf_findFun(Rf_install(c->handler), R_GlobalEnv);
	PROTECT(rhandler);

	PROTECT(rhandler_exp = allocVector(LANGSXP,2));

	SETCAR(rhandler_exp,rhandler);
	SETCAR(CDR(rhandler_exp),(*RA_new_request_rec)(r));

	UNPROTECT(2);

	errorOccurred=1;
	val = R_tryEval(rhandler_exp,NULL,&errorOccurred);

	/* Clean up reading */
	if (MR_BBin){
		apr_brigade_cleanup(MR_BBin);
		apr_brigade_destroy(MR_BBin);
	}

	/* Clean up writing */
	if (MR_BBout){
		if(!APR_BRIGADE_EMPTY(MR_BBout)){
			ap_filter_flush(MR_BBout,r->output_filters);
		}
		apr_brigade_cleanup(MR_BBout);
		apr_brigade_destroy(MR_BBout);
	}

	/* And request_rec */
	MR_Request = NULL;

	if (errorOccurred){
		if (MR_OutputErrors){
			ap_fputs(r->output_filters,MR_BBout,MR_ErrorPrefix);
			ap_fprintf(r->output_filters,MR_BBout,"error in handler: %s\nfile:%s .",c->handler,c->file);
			ap_fputs(r->output_filters,MR_BBout,MR_ErrorSuffix);
			return DONE;
		} else {
			ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"error in handler: %s, file: %s .",c->handler,c->file);
			return HTTP_INTERNAL_SERVER_ERROR;
		}
		return (MR_OutputErrors)? DONE: HTTP_INTERNAL_SERVER_ERROR;
	}

	return mr_decode_return_value(val);
}

/*************************************************************************
 *
 * Helper functions
 *
 *************************************************************************/

static MR_cfg *mr_dir_config(const request_rec *r){
	return (MR_cfg *) ap_get_module_config(r->per_dir_config,&R_module);
}


/* 
 * Time now to integrate the following stuff
 *
 * 1) Figure out how to set R_TempDir to the apache temp dir
 * 2) Set own signal handlers
 *    a) sigsegv - so as not to delete tmp dir or ask questions
 *    b) all the others that R sets
 * 3) Set own cleanup routine to avoid deleting temp dir and do
 *    proper cleanup
 * 4) Integrate as many apache.* functions into R functions:
 *    a) apache.read* and apache.write can definitely be morphed int
 *       read() and cat()/printf()/print(), etc.
 *    b) others?
 */
 
void mr_init(apr_pool_t *p){
	char *argv[] = {"mod_R", "--gui=none", "--slave", "--silent", "--vanilla","--no-readline"};
	int argc = sizeof(argv)/sizeof(argv[0]);

	if (MR_InitStatus != 0) return;

	MR_InitStatus = 1;

	if (apr_env_set("R_HOME",R_HOME,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not set R_HOME from mr_init!\n");
		exit(-1);
	}

	/* Don't let R set up its own signal handlers */
	R_SignalHandlers = 0;

	#ifdef CSTACK_DEFNS
	/* Don't do any stack checking */
	R_CStackLimit = -1;
	#endif

	#if (R_VERSION >= R_240) /* we'll take out the ifdef after R 2.4.0 release */
	mr_InitTempDir(p);
	#endif

	Rf_initEmbeddedR(argc, argv);

	/* Set R's tmp dir to apache's */
	#if (R_VERSION < R_240) /* we can delete this code after R 2.4.0 release */
	mr_InitTempDir(p);
	#endif

	/* Do we really need this */
	R_Consolefile = NULL;
	R_Outputfile = NULL;

	/* Now load RApache library */
	if (!mr_call_fun1str("library","RApache")){
		fprintf(stderr,"Fatal Error: library(\"Rapache\") failed!\n");
		exit(-1);
	}
}

static int mr_decode_return_value(SEXP ret)
{
	if (IS_INTEGER(ret) && LENGTH(ret) == 1){
		return asInteger(ret);
	}

	return DONE;
}

int mr_call_fun1str( char *funstr, char *argstr){
	SEXP val, expr, fun, arg;
	int errorOccurred;

	/* Call funstr */
	fun = Rf_findFun(Rf_install(funstr), R_GlobalEnv);
	PROTECT(fun);

	/* argument */
	PROTECT(arg = NEW_CHARACTER(1));
	SET_STRING_ELT(arg, 0, COPY_TO_USER_STRING(argstr));

	/* expression */
	PROTECT(expr = allocVector(LANGSXP,2));
	SETCAR(expr,fun);
	SETCAR(CDR(expr),arg);

	errorOccurred=1;
	val = R_tryEval(expr,NULL,&errorOccurred);
	UNPROTECT(3);

	return (errorOccurred)? 0:1;
}

static int mr_findFun( char *funstr){
	SEXP fun = NULL;

	fun = Rf_findFun(Rf_install(funstr), R_GlobalEnv);
	return (fun != NULL)? 1 : 0;

}

static apr_status_t MR_child_exit(void *data){
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

static int mr_check_cfg(request_rec *r, MR_cfg *c){
	apr_finfo_t finfo;
	apr_time_t *file_mtime;
	if (c == NULL){
		if (MR_OutputErrors){
			ap_fputs(r->output_filters,MR_BBout,MR_ErrorPrefix);
			ap_fprintf(r->output_filters,MR_BBout,"config is NULL.");
			ap_fputs(r->output_filters,MR_BBout,MR_ErrorSuffix);
		} else {
			ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"config is NULL");
		}
		return 0;
	}

	/* Grab modified time of file */
	file_mtime = (apr_time_t *)apr_hash_get(MR_Scripts,c->file,APR_HASH_KEY_STRING);

	if (c->reload || !(*file_mtime)){ /* File not sourced yet */

		if (apr_stat(&finfo,c->file,APR_FINFO_MTIME,r->pool) != APR_SUCCESS){
			if (MR_OutputErrors){
				ap_fputs(r->output_filters,MR_BBout,MR_ErrorPrefix);
				ap_fprintf(r->output_filters,MR_BBout,"%s: file not found.",c->file);
				ap_fputs(r->output_filters,MR_BBout,MR_ErrorSuffix);
			} else {
				ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"%s: file not found!",c->file);
			}
			return 0;
		}
		if (*file_mtime < finfo.mtime){
			if (!mr_call_fun1str("source",c->file)){
				if (MR_OutputErrors){
					ap_fputs(r->output_filters,MR_BBout,MR_ErrorPrefix);
					ap_fprintf(r->output_filters,MR_BBout,"source(%s) failed.",c->file);
					ap_fputs(r->output_filters,MR_BBout,MR_ErrorSuffix);
				} else {
					ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"source(%s) failed.",c->file);
				}
				return 0;
			}
		}
		*file_mtime = finfo.mtime;
	}

	/* Make sure function is found */
	if (!mr_findFun(c->handler)){
		if (MR_OutputErrors){
			ap_fputs(r->output_filters,MR_BBout,MR_ErrorPrefix);
			ap_fprintf(r->output_filters,MR_BBout,"handler %s not found in %s .",c->handler,c->file);
			ap_fputs(r->output_filters,MR_BBout,MR_ErrorSuffix);
		} else {
			ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"handler %s not found in %s .",c->handler,c->file);
		}
		return 0;
	}

	return 1;
}

void mr_init_pool(void){

	if (MR_Pool) return;

	if (apr_pool_create(&MR_Pool,NULL) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not apr_pool_create(MR_Pool)!\n");
		exit(-1);
	}

	MR_Scripts = apr_hash_make(MR_Pool);

	if (!MR_Scripts){
		fprintf(stderr,"Fatal Error: could not apr_hash_make() from MR_Pool!\n");
		exit(-1);
	}
}

/*
 * This is a bit of magic. Upon startup, Apache parses its config file twice, and
 * we really only want to do useful stuff on the second pass, so we use an apr pool
 * feature called apr_pool_userdata_[set|get]() to store state from 1 config pass
 * to the next.
 */
void mr_init_config_pass(apr_pool_t *p){
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

void mr_InitTempDir(apr_pool_t *p)
{
	const char *tmp;

	#if (R_VERSION < R_240) /* we can delete this code after R 2.4.0 release */
	if (R_TempDir){
		/* Undo R's InitTempdir() and do something sane */
		if (apr_dir_remove (R_TempDir, p) != APR_SUCCESS){
			fprintf(stderr,"Fatal Error: could not remove R's TempDir: %s!\n",R_TempDir);
			exit(-1);
		}
	}
	#endif

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

/* stdout */
int mr_stdout_vfprintf(Rconnection con, const char *format, va_list ap){
	apr_status_t rv;
	rv = apr_brigade_vprintf(MR_BBout, ap_filter_flush, MR_Request->output_filters, format, ap);
	return (rv == APR_SUCCESS)? 0 : 1;
}
int mr_stdout_fflush(Rconnection con){
	ap_filter_flush(MR_BBout,MR_Request->output_filters);
	return 0;
}
size_t mr_stdout_write(const void *ptr, size_t size, size_t n, Rconnection con){
	apr_status_t rv;
	rv = apr_brigade_write(MR_BBout, ap_filter_flush, MR_Request->output_filters, (const char *)ptr, size*n);
	return (rv == APR_SUCCESS)? n : 1;
}

/* stderr */
int mr_stderr_vfprintf(Rconnection con, const char *format, va_list ap){
	ap_log_rerror(APLOG_MARK,APLOG_ERR,0,MR_Request,"%s",apr_pvsprintf(MR_Request->pool,format,ap));
	return 0;
}

int mr_stderr_vfprintf_OutputErrors(Rconnection con, const char *format, va_list ap){
	apr_status_t rv;
	ap_fputs(MR_Request->output_filters,MR_BBout,MR_ErrorPrefix);
	rv = apr_brigade_vprintf(MR_BBout, ap_filter_flush, MR_Request->output_filters, format, ap);
	ap_fputs(MR_Request->output_filters,MR_BBout,MR_ErrorSuffix);
	return (rv == APR_SUCCESS)? 0 : 1;
}
int mr_stderr_fflush(Rconnection con){
	return 0;
}

/* We assume that buf is null-terminated */
void mr_WriteConsole(char *buf, int size){
	ap_log_rerror(APLOG_MARK,APLOG_ERR,0,MR_Request,"%s",buf);
}

void mr_WriteConsole_OutputErrors(char *buf, int size){
	request_rec *r = MR_Request; 

	ap_fputs(r->output_filters,MR_BBout,MR_ErrorPrefix);
	ap_fputs(r->output_filters,MR_BBout,buf);
	ap_fputs(r->output_filters,MR_BBout,MR_ErrorSuffix);
}
