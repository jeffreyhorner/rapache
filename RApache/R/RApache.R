##############################################################################
#
#  Copyright 2005  The Apache Software Foundation
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
##############################################################################

# ra_request_rec subsetting and class methods
"$.ra_request_rec"     <- function(x,i) .Call("RA_request_rec_idx",x,i,PACKAGE="RApache")
"$<-.ra_request_rec"   <- function(x,i,value=NULL) x
"[[.ra_request_rec"    <- function(x,i) .Call("RA_request_rec_idxc",x,i,PACKAGE="RApache")
"[[<-.ra_request_rec"  <- function(x,i,value=NULL) x
"[.ra_request_rec"     <- function(x,...) NULL
names.ra_request_rec   <- function(x)   .Call("RA_request_rec_names",x,PACKAGE="RApache")
length.ra_request_rec  <- function(x)   .Call("RA_request_rec_length",x,PACKAGE="RApache")

# Make a copy of the entire request
as.list.ra_request_rec <- function(x,...){
    lr <- list();
    for (name in names(r)){
        if (class(r[[name]]) == "apr_table")
            lr[[name]] <- as.list(r[[name]])
        else
            lr[[name]] <- r[[name]]
    }
    lr
}

# apr_table_t subsetting and class methods
"$.apr_table"     <- function(x,i) .Call("RA_apr_table_idx",x,i,PACKAGE="RApache")
"$<-.apr_table"   <- function(x,header,value=NULL) 
                                   .Call("RA_apr_table_set" ,x,header,value,PACKAGE="RApache")
"[[.apr_table"    <- function(x,i) .Call("RA_apr_table_idxc",x,i,PACKAGE="RApache")
"[[<-.apr_table"  <- function(x,header,value=NULL) 
                                   .Call("RA_apr_table_set",x,header,value,PACKAGE="RApache")
"[.apr_table"     <- function(x,...) NULL
names.apr_table   <- function(x)   .Call("RA_apr_table_names",x,PACKAGE="RApache")
length.apr_table  <- function(x)   .Call("RA_apr_table_length",x,PACKAGE="RApache")

# Make a copy of the table as a list
as.list.apr_table <- function(x,...){
    lt <- list()
    for (name in names(x)) lt[[name]] <- x[[name]]
    lt
}

# IO functions
#
apache.read     <- function(r,len=0)  .Call("RA_read",r,as.integer(len),PACKAGE="RApache");
apache.readline     <- function(r,len=0)  .Call("RA_readline",r,as.integer(len),PACKAGE="RApache");
apache.write    <- function(r,...,flush=TRUE) .Call("RA_write",r,paste(...),flush,PACKAGE="RApache")

# CGI functions
apache.get_args <- function(r) .Call("RA_get_args",r,PACKAGE="RApache");
apache.get_post <- function(r)  .Call("RA_get_post",r,PACKAGE="RApache");
apache.get_cookies  <- function(r) .Call("RA_get_cookies",r,PACKAGE="RApache");
apache.get_uploads <- function (r) .Call("RA_get_uploads",r,PACKAGE="RApache");

# HTTP header stuff
apache.add_header <- function(r,name,value){
	if (!is.character(name) || length(name)!=1)
		return(NULL)
	if (length(value)!=1)
		return(NULL)

	r$headers_out[[name]] <- as.character(value)
}
apache.set_content_type <- function(r,type=NULL){
    if (!is.null(type))
        .Call("RA_ap_set_content_type",r,type,PACKAGE="RApache")
    r$content_type
}
apache.add_cookie <- function(r,name=NULL,value="",expires=NULL,path=NULL,domain=NULL,...) {
    if (is.null(name)){
       return(NULL)
    } else {
		if (is.null(value))
			value=""
        if (length(value)!=1)
			value=""
        cookie <- paste(name,as.character(value),sep="=")
    }
    if (!is.null(expires)){
        edate  <- format(expires,"%A, %d-%b-%Y %H:%M:%S GMT",tz="GMT") 
        cookie <- paste(cookie,paste("expires",edate,sep="="),sep=";")
    }
    if (!is.null(path)){
        cookie <- paste(cookie,paste("path",path,sep="="),sep=";")
    }
    if (!is.null(domain)){
        cookie <- paste(cookie,paste("domain",domain,sep="="),sep=";")
    }

    args <- list(...)
    argnames <- names(args)
    for (i in args){
        if (argnames[i] != "")
            cookie <- paste(cookie,paste(argnames[i],args[[i]],sep="="),sep=";")
        else
            cookie <- paste(cookie,args[[i]])
    }

    if (is.null(r$headers_out$"Cache-Control"))
        apache.add_header(r,"Cache-Control",'nocache="set-cookie"')

    apache.add_header(r,"Set-Cookie",cookie)
}

# encoding and decoding of strings containing HTML Entities
apache.encode <- function (str) .Call("RA_endecode",str,TRUE,PACKAGE="RApache");
apache.decode <- function (str) .Call("RA_endecode",str,FALSE,PACKAGE="RApache");



# Handler Return values
DONE                               <- as.integer(-2)
DECLINED                           <- as.integer(-1)
OK                                 <- as.integer(0)
HTTP_CONTINUE                      <- as.integer(100)
HTTP_SWITCHING_PROTOCOLS           <- as.integer(101)
HTTP_PROCESSING                    <- as.integer(102)
HTTP_OK                            <- as.integer(200)
HTTP_CREATED                       <- as.integer(201)
HTTP_ACCEPTED                      <- as.integer(202)
HTTP_NON_AUTHORITATIVE             <- as.integer(203)
HTTP_NO_CONTENT                    <- as.integer(204)
HTTP_RESET_CONTENT                 <- as.integer(205)
HTTP_PARTIAL_CONTENT               <- as.integer(206)
HTTP_MULTI_STATUS                  <- as.integer(207)
HTTP_MULTIPLE_CHOICES              <- as.integer(300)
HTTP_MOVED_PERMANENTLY             <- as.integer(301)
HTTP_MOVED_TEMPORARILY             <- as.integer(302)
HTTP_SEE_OTHER                     <- as.integer(303)
HTTP_NOT_MODIFIED                  <- as.integer(304)
HTTP_USE_PROXY                     <- as.integer(305)
HTTP_TEMPORARY_REDIRECT            <- as.integer(307)
HTTP_BAD_REQUEST                   <- as.integer(400)
HTTP_UNAUTHORIZED                  <- as.integer(401)
HTTP_PAYMENT_REQUIRED              <- as.integer(402)
HTTP_FORBIDDEN                     <- as.integer(403)
HTTP_NOT_FOUND                     <- as.integer(404)
HTTP_METHOD_NOT_ALLOWED            <- as.integer(405)
HTTP_NOT_ACCEPTABLE                <- as.integer(406)
HTTP_PROXY_AUTHENTICATION_REQUIRED <- as.integer(407)
HTTP_REQUEST_TIME_OUT              <- as.integer(408)
HTTP_CONFLICT                      <- as.integer(409)
HTTP_GONE                          <- as.integer(410)
HTTP_LENGTH_REQUIRED               <- as.integer(411)
HTTP_PRECONDITION_FAILED           <- as.integer(412)
HTTP_REQUEST_ENTITY_TOO_LARGE      <- as.integer(413)
HTTP_REQUEST_URI_TOO_LARGE         <- as.integer(414)
HTTP_UNSUPPORTED_MEDIA_TYPE        <- as.integer(415)
HTTP_RANGE_NOT_SATISFIABLE         <- as.integer(416)
HTTP_EXPECTATION_FAILED            <- as.integer(417)
HTTP_UNPROCESSABLE_ENTITY          <- as.integer(422)
HTTP_LOCKED                        <- as.integer(423)
HTTP_FAILED_DEPENDENCY             <- as.integer(424)
HTTP_UPGRADE_REQUIRED              <- as.integer(426)
HTTP_INTERNAL_SERVER_ERROR         <- as.integer(500)
HTTP_NOT_IMPLEMENTED               <- as.integer(501)
HTTP_BAD_GATEWAY                   <- as.integer(502)
HTTP_SERVICE_UNAVAILABLE           <- as.integer(503)
HTTP_GATEWAY_TIME_OUT              <- as.integer(504)
HTTP_VERSION_NOT_SUPPORTED         <- as.integer(505)
HTTP_VARIANT_ALSO_VARIES           <- as.integer(506)
HTTP_INSUFFICIENT_STORAGE          <- as.integer(507)
HTTP_NOT_EXTENDED                  <- as.integer(510)

apache.log_error <- function(r,msg,level=APLOG_ERR) {
    .Call("RA_log_error",r,msg,level,PACKAGE="RApache")
}
# Apache Log Levels
APLOG_EMERG   <- as.integer( 0)
APLOG_ALERT   <- as.integer( 1)
APLOG_CRIT    <- as.integer( 2)
APLOG_ERR     <- as.integer( 3)
APLOG_WARNING <- as.integer( 4)
APLOG_NOTICE  <- as.integer( 5)
APLOG_INFO    <- as.integer( 6)
APLOG_DEBUG   <- as.integer( 7)

# apache.allow_methods() 
# This routine is for letting HTTP clients know exactly which methods this
# URL will handle. Modules should DECLINE request methods they do not
# handle and call apache.allow_methods(). 

apache.allow_methods <- function(r,replace=TRUE,...){
    .Call("RA_allow_methods",r,replace,list(...),PACKAGE="RApache")
}

# Methods recognized by Apache 
M_GET                   <- as.integer( 0 )  # RFC 2616: HTTP
M_PUT                   <- as.integer( 1 )
M_POST                  <- as.integer( 2 )
M_DELETE                <- as.integer( 3 )
M_CONNECT               <- as.integer( 4 )
M_OPTIONS               <- as.integer( 5 )
M_TRACE                 <- as.integer( 6 )  # RFC 2616: HTTP
M_PATCH                 <- as.integer( 7 )
M_PROPFIND              <- as.integer( 8 )  # RFC 2518: WebDAV
M_PROPPATCH             <- as.integer( 9 )
M_MKCOL                 <- as.integer( 10 )
M_COPY                  <- as.integer( 11 )
M_MOVE                  <- as.integer( 12 )
M_LOCK                  <- as.integer( 13 )
M_UNLOCK                <- as.integer( 14 ) # RFC2518: WebDAV
M_VERSION_CONTROL       <- as.integer( 15 ) # RFC3253: WebDAV Versioning
M_CHECKOUT              <- as.integer( 16 )
M_UNCHECKOUT            <- as.integer( 17 )
M_CHECKIN               <- as.integer( 18 )
M_UPDATE                <- as.integer( 19 )
M_LABEL                 <- as.integer( 20 )
M_REPORT                <- as.integer( 21 )
M_MKWORKSPACE           <- as.integer( 22 )
M_MKACTIVITY            <- as.integer( 23 )
M_BASELINE_CONTROL      <- as.integer( 24 )
M_MERGE                 <- as.integer( 25 )
M_INVALID               <- as.integer( 26 ) # RFC3253: WebDAV Versioning

# Functions for turning variables into readable html
as.html <- function(x) UseMethod("as.html")
as.html.default <- function(x) paste("(",as.character(x),")",sep="",collapse=",")
as.html.list <- function(x){
	return(
			paste("<ul>",
				paste(
					"<li>",
					lapply(
						names(x),
						function(name,x) paste(name,":",as.html(x[[name]])),
						x
						),
					"</li>",
					collapse=""
					),
				"</ul>"
				)
		  )
}
as.html.ra_request_rec <- as.html.list
as.html.apr_table <- as.html.list

# apache.gdlib_ioctx()
# 
# This function is written for a modified version of Simon Urbanek's device
# independent graphics package GDD 0.1-7, an interface to the c library
# libgd. While libgd allows output to go to a custom IO context, GDD() only
# allows output to a file. In order to let RApache handlers output dynamic
# images, I modified GDD() to accept a ctx variable which holds an external
# pointer to a libgd IO context, thus when a plot is made and the device is
# closed, the output will be sent to the IO context. apache.gdlib_ioctx()
# creates the the context given the current request r. Use it like this:
#
# GD(ctx=apache.gdlib_ioctx(r))
# ...  plot some stuff ...
# dev.off()
#
# Now output is sent to the browser. Once R allows packages access to
# the connection pool, and once GDD() can accept R connections instead
# of file names, then this function will be irrelevant and go away.
#
# Otherwise the alternative is to allow GDD() to write to a temporary file,
# then have the RApache handler open that file and send to the browser,
# thus taking a performance hit.
#
apache.gdlib_ioctx <- function(r) .Call("RA_gdlib_ioctx",r,PACKAGE="RApache")

# apache.add_common_vars()
#
# Not sure this will be needed in RApache.
# The Apache functions ap_add_common_vars() and ap_add_cgi_vars() were probably written
# for mod_cgi which needed to set up a custom sub process environment before forking and
# execing the cgi program. All the stuff in the sub process environment can be found
# in the request_rec struct.
#apache.add_common_vars <- function(r) .Call("RA_ap_add_common_vars",r,PACKAGE="RApache");

