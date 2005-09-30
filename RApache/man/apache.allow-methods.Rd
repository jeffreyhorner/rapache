\name{apache.allow-methods}
\alias{apache.allow_methods}
\title{RApache Methods}
\description{
	List of methods recognized but not necessarily handled by the Apache server.
}
\usage{
apache.allow_methods(r, replace = TRUE, ...)
}
\arguments{
\item{r}{\code{\link{ra_request_rec}} object.}
\item{replace}{Logical. whether or not to replace the current list of methods.}
\item{...}{List of methods the handler will handle.}
}
\details{
	This function is for letting HTTP clients know exactly which
	methods this handler will handle. Modules should DECLINE request
	methods they do not handle and call 
	\code{\link{apache.allow_methods}}.
}
\value{
	NULL. (Need to handle errors here.)
}
\section{List of Methods}{
M\_GET\cr
M\_PUT\cr
M\_POST\cr
M\_DELETE\cr
M\_CONNECT\cr
M\_OPTIONS\cr
M\_TRACE\cr
M\_PATCH\cr
M\_PROPFIND\cr
M\_PROPPATCH\cr
M\_MKCOL\cr
M\_COPY\cr
M\_MOVE\cr
M\_LOCK\cr
M\_UNLOCK\cr
M\_VERSION\_CONTROL\cr
M\_CHECKOUT\cr
M\_UNCHECKOUT\cr
M\_CHECKIN\cr
M\_UPDATE\cr
M\_LABEL\cr
M\_REPORT\cr
M\_MKWORKSPACE\cr
M\_MKACTIVITY\cr
M\_BASELINE\_CONTROL\cr
M\_MERGE\cr
M\_INVALID\cr
}
\section{Examples of Use}{
\preformatted{
# Sample header that only accepts GET 
handler <- function(r){
	if (r$method_number != M_GET || r$method_number != M_POST){
		apache.allow_methods(r,M_GET,M_POST)
		return(DECLINE)
	}

	# Otherwise do something
	OK
}
}
}
\keyword{programming}
\alias{M_GET}
\alias{M_PUT}
\alias{M_POST}
\alias{M_DELETE}
\alias{M_CONNECT}
\alias{M_OPTIONS}
\alias{M_TRACE}
\alias{M_PATCH}
\alias{M_PROPFIND}
\alias{M_PROPPATCH}
\alias{M_MKCOL}
\alias{M_COPY}
\alias{M_MOVE}
\alias{M_LOCK}
\alias{M_UNLOCK}
\alias{M_VERSION_CONTROL}
\alias{M_CHECKOUT}
\alias{M_UNCHECKOUT}
\alias{M_CHECKIN}
\alias{M_UPDATE}
\alias{M_LABEL}
\alias{M_REPORT}
\alias{M_MKWORKSPACE}
\alias{M_MKACTIVITY}
\alias{M_BASELINE_CONTROL}
\alias{M_MERGE}
\alias{M_INVALID}
