\name{intro}
\alias{intro}
\alias{RApache}
\title{Introduction to RApache}
\description{
	RApache is an R package distributed with mod\_R, the R/Apache
	integration project. It provides the application programming interface
	for developing web applications in R.
}
\details{
	The Apache 2.0 module interface allows module designers the
	opportunity to manage all phases of an HTTP request, from
	authentication to sending the response to the browser. Modules
	like mod\_perl, mod\_python, mod\_ruby, and php let programmers
	implement each of these phases in their respective scripting
	languages. mod\_R currently lets R programmers implement the
	response generation phase, and as the project matures, other phases
	can be exposed.

	In order to take over the response generation phase, the
	Apache configuration file must map a URL to an R function. In
	the simplest case this can be accomplished by the following
	example. This little snippet goes into the Apache config file:\preformatted{

<Location /test/R>
	SetHandler r-handler
	Rsource /absolute/path/test.R
	RreqHandler handler
</Location>
}

	The Apache \code{Location} directive specifies what happens when
	the web server needs to fulfill the following request:

	http://hostname/test/R

	The Apache \code{SetHandler} directive specifies the modules
	that will handle the request. The string \code{r-handler} is
	case sensitive so make sure you spell it correctly. It must be
	specified to allow mod\_R to take over the request. \code{Rsource}
	is a mod\_R directive that specifies an R file to \code{source()}
	upon server startup, and \code{RreqHandler} specifies the R
	function to execute to fulfill the request. In this example, the
	function \code{handler} must have been defined in the file test.R.

	All handlers must be defined as a function taking one argument and
	return a valid HTTP response code. The following is a \code{hello
	world} example:\preformatted{

handler <- function(r){
	apache.write(r,"<H1>Hello from mod_R</H1>")
	OK
}
}
	The variable \code{r} is the RApache
	request record and behaves similarly to a list. See
	\code{\link{request_rec}} for more details. (This is where
	the RApache package comes in. It is silently loaded when
	Apache starts. Thus, one doesn't need to call
	\code{library(RApache)} before defining the handler.)

	Each Apache function is prefixed with the string \code{apache.} and
	takes the RApache request record as the first argument.

	\emph{One thing to note:} R currently does not expose the internals of
	connection streams, thus it is hard to write custom functions for
	R generics such as \code{\link{readLines}}, \code{\link{write}},
	etc. Until then, RApache will continue with the current scheme,
	similar to other scripting language modules.

	The \code{\link{apache.write}} function sends its output to the browser.
	\code{\link{OK}} is the return code that tells Apache the handler is done with
	it's part of the request. Other return codes are defined in \code{\link{return_codes}}.
}
\seealso{
	\item{\code{\link{request_rec}}}{Details about the RApache request object}
	\item{\code{\link{return_codes}}}{Valid handler return codes}
	\item{\code{\link{directives}}}{mod\_R directives placed in the Apache config file}
	\item{\code{\link{apr_table}}}{Details about the R apr\_table objects}
	\item{\code{\link{IO}}}{Ways of reading and writing HTTP requests}
}
\keyword{programming}
