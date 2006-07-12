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
#define MOD_R_VERSION "mod_R/0.1.2"
#include "mod_R.h" 

#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>

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
#include <Rdefines.h>
#include <Rinterface.h>

/*
 * Define some internal R library stuff to make embedding less
 * harmful
 */

/* Override R's setting of temporary directory */
extern char *R_TempDir;

/*************************************************************************
 *
 * Globals
 *
 *************************************************************************/

/* Functions exported by RApache which we need to find at RApache load time */
SEXP (*RA_new_request_rec)(request_rec *r);

/* Number of times apache has parsed config files; we'll do stuff on second pass */
static int MR_config_pass = 1;

/* Don't start R more than once, if the flag is 1 it's already started */
static int MR_init_status = 0;

/* Cache child pid on startup. If we fork() we don't want to do certain
 * cleanup steps, especially on cgi fork
 */
static unsigned long MR_child_pid;

/* Per-child memory for configuration */
static apr_pool_t *MR_pool = NULL;
static apr_hash_t *MR_cfg_libs = NULL;
static apr_hash_t *MR_cfg_scripts = NULL;

/*
 * Brigades for reading and writing to client
 * exported to RApache
 */
apr_bucket_brigade *MR_bbin;
apr_bucket_brigade *MR_bbout;

/*************************************************************************
 *
 * R module types
 *
 *************************************************************************/
typedef struct {
	int loaded;
} MR_cfg_persist;

typedef struct MR_cfg {
	char *library;
	char *script;
	char *reqhandler;
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
extern int Rf_initEmbeddedR(int argc, char *argv[]);

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
static const char *MR_cmd_req_handler(cmd_parms *cmd, void *conf, const char *val);
static const char *MR_cmd_source(cmd_parms *cmd, void *conf, const char *val);
static const char *MR_cmd_library(cmd_parms *cmd, void *conf, const char *val);

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

/*************************************************************************
 *
 * Command array
 *
 *************************************************************************/

static const command_rec MR_cmds[] = {
	AP_INIT_TAKE1("RreqHandler", MR_cmd_req_handler, NULL, OR_OPTIONS, "R Function to handle request"),
	AP_INIT_TAKE1("Rsource", MR_cmd_source, NULL, OR_OPTIONS, "R file to be sourced at startup"),
	AP_INIT_TAKE1("Rlibrary", MR_cmd_library, NULL, OR_OPTIONS, "R library to be loaded at startup"),
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
 * Module functions
 *
 *************************************************************************/

static void *MR_create_dir_cfg(apr_pool_t *p, char *dir){
	MR_cfg *cfg;

	cfg = (MR_cfg *)apr_pcalloc(p,sizeof(MR_cfg));

	return (void *)cfg;
}

void *MR_merge_dir_cfg(apr_pool_t *p, void *parent, void *new){
	MR_cfg *cfg;
	MR_cfg *pcfg = (MR_cfg *)parent;
	MR_cfg *ncfg = (MR_cfg *)new;

	cfg = (MR_cfg *)apr_pcalloc(p,sizeof(MR_cfg));

	/* Add parent stuff, even if null */
	cfg->library = pcfg->library;
	cfg->script = pcfg->script;
	cfg->reqhandler = pcfg->reqhandler;

	/* Now add new config stuff, overriding parent */
	cfg->library = ncfg->library;
	cfg->script = ncfg->script;
	cfg->reqhandler = ncfg->reqhandler;

	return (void *)cfg;
}

void *MR_create_srv_cfg(apr_pool_t *p, server_rec *s){
	MR_cfg *cfg;

	mr_init_config_pass(s->process->pool);

	cfg = (MR_cfg *)apr_pcalloc(p,sizeof(MR_cfg));

	return (void *)cfg;
}

void *MR_merge_srv_cfg(apr_pool_t *p, void *parent, void *new){
	MR_cfg *cfg;
	MR_cfg *pcfg = (MR_cfg *)parent;
	MR_cfg *ncfg = (MR_cfg *)new;

	cfg = (MR_cfg *)apr_pcalloc(p,sizeof(MR_cfg));

	/* Add parent stuff, even if null */
	cfg->library = pcfg->library;
	cfg->script = pcfg->script;
	cfg->reqhandler = pcfg->reqhandler;

	/* Now add new config stuff, overriding parent */
	cfg->library = ncfg->library;
	cfg->script = ncfg->script;
	cfg->reqhandler = ncfg->reqhandler;

	return (void *)cfg;
}


static void MR_register_hooks (apr_pool_t *p)
{
	ap_hook_post_config(MR_hook_post_config, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_child_init(MR_hook_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(MR_hook_request_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/*************************************************************************
 *
 * Command functions
 *
 *************************************************************************/

static const char *MR_cmd_req_handler(cmd_parms *cmd, void *conf, const char *val){
	MR_cfg *cfg = (MR_cfg *)conf;

	cfg->reqhandler = apr_pstrdup(cmd->pool,val);

	return NULL;
}

static const char *MR_cmd_source(cmd_parms *cmd, void *conf, const char *val){
	MR_cfg *cfg = (MR_cfg *)conf;

	cfg->script = apr_pstrdup(cmd->pool,val);

	if (MR_config_pass == 1){
		apr_finfo_t finfo;
		if (apr_stat(&finfo,val,APR_FINFO_TYPE,cmd->pool) != APR_SUCCESS){
			return apr_psprintf(cmd->pool,"Rsource: %s file not found!",val);
		}
		return NULL;
	}

	/* Cache all script files in persistent config pool.
	 * We'll test to see if we've sourced these in the request handler
	 */
	mr_init_pool();
	apr_hash_set(MR_cfg_scripts,cfg->script,APR_HASH_KEY_STRING,apr_pcalloc(MR_pool,sizeof(MR_cfg_persist)));

	return  NULL;
}

static const char *MR_cmd_library(cmd_parms *cmd, void *conf, const char *val){
	MR_cfg *cfg = (MR_cfg *)conf;

	cfg->library = apr_pstrdup(cmd->pool,val);

	if (MR_config_pass == 1) return NULL;

	/* Cache all library names in persistent config pool.
	 * We'll test to se if we've called library() on these in the request handler
	 */
	mr_init_pool();
	apr_hash_set(MR_cfg_libs,cfg->library,APR_HASH_KEY_STRING,apr_pcalloc(MR_pool,sizeof(MR_cfg_persist)));

	return NULL;
}

/*************************************************************************
 *
 * Hook functions
 *
 *************************************************************************/

static int MR_hook_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s){
	ap_add_version_component(pconf,MOD_R_VERSION);
	return OK;
}

static void MR_hook_child_init(apr_pool_t *p, server_rec *s){
	MR_child_pid=(unsigned long)getpid();
	mr_init(p);
	apr_pool_cleanup_register(p, p, MR_child_exit, MR_child_exit);
}

static int MR_hook_request_handler (request_rec *r)
{
	MR_cfg *cfg;

	SEXP val, rhandler_exp, rhandler;
	int errorOccurred;

	// Only handle if correct handler
	if (strcmp(r->handler,"r-handler") != 0)
		return DECLINED;

	/* Let the Apache config or the handler determine
	 * the type
	 */
	/* ap_set_content_type(r,"text/html"); */

	/* Config */
	cfg = mr_dir_config(r);

	/* 
	 * Bad apache config? 
	 *
	 * Here, we find the handler we must run and
	 * load any scripts or libraries.
	 */
	if (!mr_check_cfg(r,cfg))
		return DECLINED;

	/* Init reading */
	MR_bbin = NULL;

	/* Init writing */
	MR_bbout = apr_brigade_create(r->pool, r->connection->bucket_alloc);

	/*
	 * Now call (cfg->reqhandler)(req)
	 */

	rhandler = Rf_findFun(Rf_install(cfg->reqhandler), R_GlobalEnv);
	PROTECT(rhandler);

	PROTECT(rhandler_exp = allocVector(LANGSXP,2));

	SETCAR(rhandler_exp,rhandler);
	SETCAR(CDR(rhandler_exp),(*RA_new_request_rec)(r));

	UNPROTECT(2);

	errorOccurred=1;
	val = R_tryEval(rhandler_exp,NULL,&errorOccurred);

	/* Clean up reading */
	if (MR_bbin){
		apr_brigade_cleanup(MR_bbin);
		apr_brigade_destroy(MR_bbin);
	}

	/* Clean up writing */
	if (MR_bbout){
		if (!APR_BRIGADE_EMPTY(MR_bbout)) ap_filter_flush(MR_bbout,r->output_filters);
		apr_brigade_cleanup(MR_bbout);
		apr_brigade_destroy(MR_bbout);
	}


	if (errorOccurred){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"mod_R: tryEval(%s) failed!",r->filename);
		return HTTP_INTERNAL_SERVER_ERROR;
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
	const char *tmpdir;

	if (MR_init_status != 0) return;

	MR_init_status = 1;

	if (apr_env_set("R_HOME",R_HOME,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not set R_HOME from mr_init!\n");
		exit(-1);
	}

	Rf_initEmbeddedR(argc, argv);

	/* Set R's tmp dir to apache's */

	/* First remove R's version */
	if (R_TempDir){
		if (apr_dir_remove (R_TempDir, p) != APR_SUCCESS){
			fprintf(stderr,"Fatal Error: could not remove R's TempDir: %s!\n",R_TempDir);
			exit(-1);
		}
	}

	if (apr_temp_dir_get(&tmpdir,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: apr_temp_dir_get() failed!\n");
		exit(-1);
	}

	R_TempDir = (char *)tmpdir;

	if (apr_env_set("R_SESSION_TMPDIR",R_TempDir,p) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not set up R_SESSION_TMPDIR!\n");
		exit(-1);
	}

	/* Set own signal handlers */

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
	apr_pool_t *p = (apr_pool_t *)data;
	char *Rtmp;
	/* R_dot_Last(); */
	unsigned long pid;

	pid = (unsigned long)getpid();

	/* Only run if we've actually initialized AND
	 * we haven't forked (for mod_cgi).
	 */
	if (MR_init_status && MR_child_pid == pid){
		MR_init_status = 0;
		/* R_RunExitFinalizers(); */
		/* Rf_CleanEd(); */
		/* KillAllDevices(); */
	}

	if (MR_pool) {
		apr_pool_destroy(MR_pool);
		MR_pool = NULL;
	}

	return APR_SUCCESS;
}

static int mr_check_cfg(request_rec *r, MR_cfg *cfg){
	MR_cfg_persist *cfg_persist;
	if (cfg == NULL){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"mr_check_cfg(): cfg is NULL");
		return 0;
	}

	if (cfg->library){
		cfg_persist = (MR_cfg_persist *)apr_hash_get(MR_cfg_libs,cfg->library,APR_HASH_KEY_STRING);
		if (cfg_persist && !cfg_persist->loaded){
			if(!mr_call_fun1str("library",cfg->library)){
				ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"mr_check_cfg(): library(%s) failed!",cfg->library);
				return 0;
			}
			cfg_persist->loaded = 1;
		}
	}

	if (cfg->script){
		cfg_persist = (MR_cfg_persist *)apr_hash_get(MR_cfg_scripts,cfg->script,APR_HASH_KEY_STRING);
		if (cfg_persist && !cfg_persist->loaded){
			if (!mr_call_fun1str("source",cfg->script)){
				ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"mr_check_cfg(): source(%s) failed!",cfg->script);
				return 0;
			}
			cfg_persist->loaded = 1;
		}
	}

	if (cfg->reqhandler && !mr_findFun(cfg->reqhandler)){
		ap_log_rerror(APLOG_MARK,APLOG_ERR,0,r,"mr_check_cfg(): RreqHandler %s not found!",cfg->reqhandler);
		return 0;
	}

	return 1;
}

void mr_init_pool(void){

	if (MR_pool) return;

	if (apr_pool_create(&MR_pool,NULL) != APR_SUCCESS){
		fprintf(stderr,"Fatal Error: could not apr_pool_create(MR_pool)!\n");
		exit(-1);
	}

	MR_cfg_libs = apr_hash_make(MR_pool);
	MR_cfg_scripts = apr_hash_make(MR_pool);

	if (!MR_cfg_libs || !MR_cfg_scripts){
		fprintf(stderr,"Fatal Error: could not apr_hash_make() from MR_pool!\n");
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
	MR_config_pass = *((int *)cfg_pass);
}
