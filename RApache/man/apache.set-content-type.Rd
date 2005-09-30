\name{apache.set-content-type}
\alias{apache.set_content_type}
\title{Setting the Content Type header}
\description{
	Allows handler to set the content type of the request.
}
\usage{
apache.set_content_type( r, type = NULL)
}
\arguments{
\item{r}{\code{\link{ra_request_rec}} object.}
\item{type}{Character vector length 1. Valid MIME type.}
}
\details{
	Must be called before first call to \code{\link{apache.write}}.
}
\value{
  \code{type} or NULL on error.
}
\keyword{programming}
