\name{apache.log-error}
\alias{apache.log_error}
\title{Logging Errors to the Apache error log}
\description{
	Function to log errors to the error log file.
}
\usage{
apache.log_error(r, msg, level = APLOG_ERR)
}
\arguments{
\item{r}{\code{\link{ra_request_rec}} object.}
\item{msg}{Character vector length 1. Message to be written to the log.}
\item{level}{Integer. One of the Apache log levels.}
}
\details{
Log levels are:\cr\cr
APLOG\_EMERG\cr
APLOG\_ALERT\cr
APLOG\_CRIT\cr
APLOG\_ERR\cr
APLOG\_WARNING\cr
APLOG\_NOTICE\cr
APLOG\_INFO\cr
APLOG\_DEBUG\cr
}
\value{
NULL
}
\keyword{programming}
\alias{APLOG_EMERG}
\alias{APLOG_ALERT}
\alias{APLOG_CRIT}
\alias{APLOG_ERR}
\alias{APLOG_WARNING}
\alias{APLOG_NOTICE}
\alias{APLOG_INFO}
\alias{APLOG_DEBUG}
