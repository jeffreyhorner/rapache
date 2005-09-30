\name{apache.add-cookie}
\alias{apache.add_cookie}
\title{Adding Cookies to Outgoing Headers}
\description{
	Add Cookies to Outgoing Headers.
}
\usage{
apache.add_cookie(r, name = NULL, value = "" ,expires = NULL, 
 path = NULL, domain = NULL, ...)
}
\arguments{
\item{r}{\code{\link{ra_request_rec}} object}
\item{name}{Character. Name of the cookie.}
\item{value}{Any valid R object of length 1 that can be converted to type character. Value of the cookie.}
\item{expires}{POSIXct object. Determines when the cookie expires.}
\item{path}{Character. URL to which the cookie pertains.}
\item{domain}{Character. Domain to which the cookie pertains.}
\item{...}{Any additional values appended to the cookie.}
}
\details{
	In the simplest case, calling
	\code{apache.add_cookie(r,''foo'',''bar'')} sets the cookie ''foo''
	to the value ''bar''. Calling \code{apache.add_cookie(r,''foo'')}
	will delete the cookie ''foo''.

	Any non-standard key value pairs can be appended by using ...
}
\value{
Either the string representation of the cookie, or NULL if name was not provided.}
\seealso{
	Original Netscape Cookie Spec:\cr
		\url{http://wp.netscape.com/newsref/std/cookie\_spec.html}\cr
	RFC2109, HTTP State Management Mechanism, original standard\cr
	RFC2964, Use of HTTP State Management\cr
	RFC2965, HTTP State Management Mechanism, latest standard\cr

}
\keyword{programming}
