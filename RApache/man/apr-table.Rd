\name{apr-table}
\alias{apr_table}
\alias{as.list.apr_table}
\alias{length.apr_table}
\alias{names.apr_table}
\alias{[.apr_table}
\alias{[[.apr_table}
\alias{[[<-.apr_table}
\alias{$.apr_table}
\alias{$<-.apr_table}
\alias{headers_in}
\alias{headers_out}
\title{Structure of the apr\_table type in RApache}
\description{
	The apr\_table type is treated similarly to an R list. Both the
    \code{$} and \code{[[} indexing operators are available, however
    partial completion
	of element names is not performed. Dispatching is done S3
	style on the class attribute \code{apr_table}.	Each element of
	an \code{apr_table} is case-insensitive. Each value is a character
	vector of length 1. 

	\code{as.list.apr_table} is available for coercion to list type.

	\code{names.apr_table} and \code{length.apr_table} are also
	available for iterating purposes.
}
\seealso{
	\code{\link{request_rec}} contains apr\_table element values
	related to HTTP headers.  \code{\link{apache.get_args}}, \code{\link{apache.get_post}}, 
	\code{\link{apache.get_cookies}}, and \code{\link{apache.get_uploads}}
	all return values of type apr\_table.
}
\keyword{programming}
