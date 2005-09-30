\name{request-rec}
\alias{request_rec}
\alias{ra_request_rec}
\alias{as.list.ra_request_rec}
\alias{length.ra_request_rec}
\alias{names.ra_request_rec}
\alias{[.ra_request_rec}
\alias{[[.ra_request_rec}
\alias{[[<-.ra_request_rec}
\alias{$.ra_request_rec}
\alias{$<-.ra_request_rec}

\title{Structure of the RApache request record}
\description{
	The RApache request record is treated similarly to an R list. Both the 
    \code{$} and \code{[[} indexing operators are available, however partial completion
	of element names is not performed. Dispatching is done S3 style on the
	class attribute \code{ra_request_rec}. This class is distinguished from the
	Apache C struct \code{request_rec}. Variables of class \code{ra_request_rec} are currently read-only, but that may change in future releases. 
	
	\code{as.list.ra_request_rec} is available for a malleable version.

	\code{names.ra_request_rec} and \code{length.ra_request_rec} are also available for iterating purposes.
}
\section{Elements of class ra\_request\_rec}{
	\item{\code{headers_in}:}{object of class \code{apr_table} (see
	\code{\link{apr_table}}) containing all the HTTP headers sent by the
	client. }

    \item{\code{headers_out}:}{ object of class \code{apr_table}
    containing all the HTTP headers to send to the client}
    \item{\code{err_headers_out}}{ object of class \code{apr_table}
    containing all the HTTP headers to send to the client}

    \item{\code{err_headers_out}:}{ object of class \code{apr_table}
    containing headers to be sent when an error occurs; they persist
    across internal redirects.}

    \item{\code{proto_num}:}{ Integer. Protocol version number of protocol;
    1.1 = 1001}

    \item{\code{protocol}:}{ Character. Protocol string, as given to us,
    or HTTP/0.9.}

    \item{\code{unparsed_uri}:}{ Character. The URI without any parsing
    performed.}

    \item{\code{uri}:}{ Character. The path portion of the URI. }

    \item{\code{canonical_filename}:}{Character. The true filename, we
    canonicalize r\$filename if these don't match. }

    \item{\code{path_info}:}{ Character. The PATH\_INFO extracted from
    this request. }

    \item{\code{args}:}{ Character. The QUERY\_ARGS extracted from this
    request. }

    \item{\code{content_type}:}{Character. The content-type for the
    current request.}

    \item{\code{handler}:}{ Character. Should always be r-handler. The
    handler string that we use to call a handler function.}

    \item{\code{content_encoding}:}{Character. How to encode the data. }

    \item{\code{range}:}{Character. The \code{Range:} header. }

    \item{\code{hostname}:}{ Character. The server hostname. }

    \item{\code{user}:}{ Character. If an authentication check was made,
    this gets set to the user name.}

    \item{\code{header_only}:}{Logical. HEAD request, as opposed to GET. }

    \item{\code{no_cache}:}{Logical. This response can not be cached. }

    \item{\code{no_local_copy}:}{ Logical. There is no local copy of this
    response. } \item{\code{assbackwards}}{Logical. HTTP/0.9, ''simple''
    request (e.g. \code{GET /foo\n} w/no headers). Developers have found
    this a useful way to internally redirect without headers.}

    \item{\code{status}:}{Integer. Status line. }

    \item{\code{method_number}:}{ Integer. M\_GET, M\_POST, etc. See
    \code{\link{apache.allow_methods}} for more details. }

    \item{\code{eos_sent}:}{Logical. A flag to determine if the eos bucket
    has been sent yet. }

    \item{\code{the_request}:}{Character. First line of the request. }

    \item{\code{method}:}{ Character. Request method (eg. GET, HEAD,
    POST, etc.)}

    \item{\code{status_line}:}{Character. Status line, if set by script. }

    \item{\code{bytes_sent}:}{Numeric. Number of bytes sent. }

    \item{\code{clength}:}{Numeric. The ''real'' content length. }

    \item{\code{remaining}:}{Numeric. Remaining bytes left to read from
    the request body. }

    \item{\code{read_length}:}{ Numeric. Number of bytes that have been
    read  from the request body. }

    \item{\code{request_time}:}{ POSIXct DateTime object. Time when the
    request started.}

    \item{\code{mtime}:}{ POSIXct DateTime object. Last modified time of
    the requested resource .}

}
\seealso{
	\item{\code{\link{apr_table}}}{Details about the R apr\_table objects}
}
\keyword{programming}
