\name{apache.add-header}
\alias{apache.add_header}
\title{Adding HTTP headers}
\description{
	Adding HTTP headers to the response.
}
\usage{
apache.add_header(r, name, value)
}
\arguments{
\item{r}{\code{\link{ra_request_rec}} object.}
\item{name}{Character. Name of the header: case-insensitive.}
\item{value}{Any valid R object of length 1 that can be converted to type character. Value of the header.}
}
\details{
	All headers must be added before the first call of \code{\link{apache.write}}.
}
\value{
	Returns the value passed in or NULL if name or value are not of type \link{character}.
}
\keyword{programming}
