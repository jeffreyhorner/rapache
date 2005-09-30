\name{as.html}
\alias{as.html}
\alias{ashtml}
\title{Converting RApache objects to HTML}
\description{
	These functions are useful for creating HTML representations of
	R objects
}
\usage{
as.html(x)
}
\arguments{
\item{x}{A \code{\link{ra_request_rec}} object, \code{\link{apr_table}} object, or a basic vector.}
}
\value{
	Returns a character vector of length 1.
}
\keyword{programming}
\alias{as.html.default}
\alias{as.html.list}
\alias{as.html.ra_request_rec}
\alias{as.html.apr_table}
