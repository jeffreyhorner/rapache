setContentType <- function(type) .Call('RApache_setContentType',type)
setHeader <- function(header,value) .Call('RApache_setHeader',header,value)
setCookie <- function(name=NULL,value='',expires=NULL,path=NULL,domain=NULL,...){
	args <- list(...)
	therest <- ifelse(length(args)>0,paste(paste(names(args),args,sep='='),collapse=';'),'')
	.Call('RApache_setCookie',name,value,expires,path,domain,therest)
}
urlEncode <- function(str) .Call('RApache_urlEnDecode',str,TRUE)
urlDecode <- function(str) .Call('RApache_urlEnDecode',str,FALSE)
sendBin <- function(object, con=stdout(), size=NA_integer_, endian=.Platform$endian){
	swap <- endian != .Platform$endian
	if (!is.vector(object) || mode(object) == 'list') 
		stop('can only write vector objects')
	.Call('RApache_sendBin',object,size,swap)
}

GET <- COOKIES <- POST <- FILES <- SERVER <- NULL

DONE <- -2
DECLINED <- -1
OK <- 0
HTTP_CONTINUE <- 100
HTTP_SWITCHING_PROTOCOLS <- 101
HTTP_PROCESSING <- 102
HTTP_OK <- 200
HTTP_CREATED <- 201
HTTP_ACCEPTED <- 202
HTTP_NON_AUTHORITATIVE <- 203
HTTP_NO_CONTENT <- 204
HTTP_RESET_CONTENT <- 205
HTTP_PARTIAL_CONTENT <- 206
HTTP_MULTI_STATUS <- 207
HTTP_MULTIPLE_CHOICES <- 300
HTTP_MOVED_PERMANENTLY <- 301
HTTP_MOVED_TEMPORARILY <- 302
HTTP_SEE_OTHER <- 303
HTTP_NOT_MODIFIED <- 304
HTTP_USE_PROXY <- 305
HTTP_TEMPORARY_REDIRECT <- 307
HTTP_BAD_REQUEST <- 400
HTTP_UNAUTHORIZED <- 401
HTTP_PAYMENT_REQUIRED <- 402
HTTP_FORBIDDEN <- 403
HTTP_NOT_FOUND <- 404
HTTP_METHOD_NOT_ALLOWED <- 405
HTTP_NOT_ACCEPTABLE <- 406
HTTP_PROXY_AUTHENTICATION_REQUIRED <- 407
HTTP_REQUEST_TIME_OUT <- 408
HTTP_CONFLICT <- 409
HTTP_GONE <- 410
HTTP_LENGTH_REQUIRED <- 411
HTTP_PRECONDITION_FAILED <- 412
HTTP_REQUEST_ENTITY_TOO_LARGE <- 413
HTTP_REQUEST_URI_TOO_LARGE <- 414
HTTP_UNSUPPORTED_MEDIA_TYPE <- 415
HTTP_RANGE_NOT_SATISFIABLE <- 416
HTTP_EXPECTATION_FAILED <- 417
HTTP_UNPROCESSABLE_ENTITY <- 422
HTTP_LOCKED <- 423
HTTP_FAILED_DEPENDENCY <- 424
HTTP_UPGRADE_REQUIRED <- 426
HTTP_INTERNAL_SERVER_ERROR <- 500
HTTP_NOT_IMPLEMENTED <- 501
HTTP_BAD_GATEWAY <- 502
HTTP_SERVICE_UNAVAILABLE <- 503
HTTP_GATEWAY_TIME_OUT <- 504
HTTP_VERSION_NOT_SUPPORTED <- 505
HTTP_VARIANT_ALSO_VARIES <- 506
HTTP_INSUFFICIENT_STORAGE <- 507
HTTP_NOT_EXTENDED <- 510

hrefify <- function(title) gsub('[\\\\.()]','_',title,perl=TRUE);
cl<-'e';
scrub <- function(st){ if (is.null(st)) return('null'); if (is.na(st)) return('NA'); if (length(st) == 0) return ('length 0 sting'); if (typeof(st) == 'closure') { sink(textConnection('stt','w')); str(st); sink(); st <- stt; } else {st  <- as.character(st) } ; st <- gsub('&','&amp;',st); st <- gsub('@','_at_',st); st <- gsub('<','&lt;',st); st <- gsub('>','&gt;',st); if (length(st) == 0 || is.null(st) || st == '') st <- '&nbsp;'; st };
zebelem <- function(n,v) { cl <<- ifelse(cl=='e','o','e'); cat('<tr class=\"',cl,'\">'); if(!is.na(n)) cat('<td class=\"l\">',n,'</td>'); cat('<td>'); if (length(v)>1) zebra(NULL,v) else cat(scrub(v)); cat('</td></tr>\n'); };
zebra <- function(title,l){ if (!is.null(title)) cat('<h2><a name=\"',hrefify(title),'\"> </a>',title,'</h2>',sep=''); cat('<table><tbody>',sep=''); n <- names(l); mapply(zebelem,if(is.null(n)) rep(NA,length(l)) else n, l); cat('</tbody></table>\n') };
 zebrifyPackage <-function(package){ zebra(package,unclass(packageDescription(package))); cat('<br/><hr/>\\n') };

RApacheInfo <- function() {
cat("<html><head>");
cat("<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />");
cat("<style type=\"text/css\">");
cat("body { font-family: \"lucida grande\",verdana,sans-serif; margin-left: 210px; margin-right: 18px; }");
cat("table { border: 1px solid #8897be; border-spacing: 0px; font-size: 10pt; }");
cat("td { border-bottom:1px solid #d9d9d9; border-left:1px solid #d9d9d9; border-spacing: 0px; padding: 3px 8px; }");
cat("td.l { font-weight: bold; width: 10%; }");
cat("tr.e { background-color: #eeeeee; border-spacing: 0px; }");
cat("tr.o { background-color: #ffffff; border-spacing: 0px; }");
cat("div a { text-decoration: none; color: white; }");
cat("a:hover { color: #8897be; background: white; }");
cat("tr:hover { background: #8897be; }");
cat("img.map { position: fixed; border: 0px; left: 50px; right: auto; top: 10px; }");
cat("div.map { background: #8897be; font-weight: bold; color: white; position: fixed; bottom: 30px; height: auto; left: 15px; right: auto; top: 110px; width: 150px; padding: 0 13px; text-align: right; font-size: 12pt; }");
cat("div.map p { font-size: 10pt; font-family: serif; font-style: italic; }");
cat("div.h { font-size: 20pt; font-weight: bold; }");
cat("h4 { font-size: 10pt; font-weight: bold; color: grey;}");
cat("hr {background-color: #cccccc; border: 0px; height: 1px; color: #000000;}");
cat("</style>");
cat("<title>RApacheInfo()</title>");
cat("<meta name=\"ROBOTS\" content=\"NOINDEX,NOFOLLOW,NOARCHIVE\" />");
cat("</head>");
cat("<body>");
cat("<a name=\"Top\"> </a>");
cat("<a href=\"http://www.r-project.org/\"><img class=\"map\" alt=\"R Language Home Page\" src=\"http://www.r-project.org/Rlogo.jpg\"/></a>");

cat("<div class=\"h\">RApache version ???</div>");

cat("<div class=\"map\">");
cat("<p>jump to:</p>");
cat("<a href=\"#Top\">Top</a><br/><hr/>");

cat("<a href=\"#"); cat(hrefify('R.version')); cat("\">R.version</a><br/>");
cat("<a href=\"#"); cat(hrefify('search()')); cat("\">search()</a><br/>");
cat("<a href=\"#"); cat(hrefify('.libPaths()')); cat("\">.libPaths()</a><br/>");
cat("<a href=\"#"); cat(hrefify('options()')); cat("\">options()</a><br/>");
cat("<a href=\"#"); cat(hrefify('Sys.getenv()')); cat("\">Sys.getenv()</a><br/>");
cat("<a href=\"#"); cat(hrefify('Sys.info()')); cat("\">Sys.info()</a><br/>");
cat("<a href=\"#"); cat(hrefify('.Machine')); cat("\">.Machine</a><br/>");
cat("<a href=\"#"); cat(hrefify('.Platform')); cat("\">.Platform</a><br/>");
cat("<a href=\"#"); cat(hrefify('Cstack_info()')); cat("\">Cstack_info()</a><br/><hr/>");

cat("<a href=\"#Attached_Packages\">Attached Packages</a><br/><hr/>");
cat("<a href=\"#Installed_Packages\">Installed Packages</a><br/><hr/>");
cat("<a href=\"#License\">License</a><br/><hr/>");
cat("<a href=\"#People\">People</a>");
cat("</div>");

zebra('R.version',R.version); cat("<br/><hr/>");
zebra('search()',search()); cat("<br/><hr/>");
zebra('.libPaths()',.libPaths()); cat("<br/><hr/>");
zebra('options()',options()); cat("<br/><hr/>");
zebra('Sys.getenv()',as.list(Sys.getenv())); cat("<br/><hr/>");
zebra('Sys.info()',as.list(Sys.info())); cat("<br/><hr/>");
zebra('.Machine',.Machine); cat("<br/><hr/>");
zebra('.Platform',.Platform); cat("<br/><hr/>");
zebra('Cstack_info()',as.list(Cstack_info())); cat("<br/><hr/>");

cat("<h1><a name=\"Attached_Packages\"></a>Attached Packages</h1>");
lapply(sub('package:','',search()[grep('package:',search())]),zebrifyPackage);
cat("<h1><a name=\"Installed_Packages\"></a>Installed Packages</h1>");
lapply(attr(installed.packages(),'dimnames')[[1]],zebrifyPackage);

cat("<h2><a name=\"License\"></a>License</h2>");
cat("<pre>");
cat("Copyright 2005  The Apache Software Foundation\n");
cat("\n");
cat("Licensed under the Apache License, Version 2.0 (the \"License\");\n");
cat("you may not use this file except in compliance with the License.\n");
cat("You may obtain a copy of the License at\n");
cat("\n");
cat("  <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">http://www.apache.org/licenses/LICENSE-2.0\"</a>\n");
cat("\n");
cat("Unless required by applicable law or agreed to in writing, software\n");
cat("distributed under the License is distributed on an \"AS IS\" BASIS,\n");
cat("WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n");
cat("See the License for the specific language governing permissions and\n");
cat("limitations under the License.\n");
cat("</pre><hr/>\n");
cat("<h2><a name=\"People\"></a>People</h2>\n");
cat("<p>Thanks to the following people for their contributions, giving advice, noticing when things were broken and such. If I've forgotten to mention you, please email me.</p>\n");
cat("<pre>\n");
cat("	Gregoire Thomas\n");
cat("	Jan de Leeuw\n");
cat("	Keven E. Thorpe\n");
cat("	Jeremy Stephens\n");
cat("	Aleksander Wawer\n");
cat("	David Konerding\n");
cat("	Robert Kofler\n");
cat("	Jeroen Ooms\n");
cat("	Michael Driscoll\n");
cat("</pre>");
cat("</body></html>");
}
