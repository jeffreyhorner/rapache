\name{IO}
\alias{IO}
\alias{apache.read}
\alias{apache.readline}
\alias{apache.write}
\alias{apache.get_args}
\alias{apache.get_post}
\alias{apache.get_cookies}
\alias{apache.get_uploads}
\title{IO in RApache}
\description{All functions related to reading or writing data from/to clients.}
\usage{
# manual reading
apache.read(r ,len = 0)
apache.readline(r ,len = 0)

# libapreq2 reading
apache.get_post(r)
apache.get_uploads(r)

apache.write(r, ..., flush = TRUE)

apache.get_args(r)
apache.get_cookies(r)
}
\arguments{
\item{r}{object of type \code{\link{ra_request_rec}}. }
\item{len}{integer. length of the buffer to fill, or in the
 case of apache.readline, the maximum length of the string to
 return. If len = 0, then this value defaults to the Apache value
 of HUGE\_STRING\_LEN which is, as of this writing, 8Kb.}
\item{...}{Arguments to be passed to \code{\link{paste}}.}
\item{flush}{logical. whether or not to flush the write buffers. (not sure if this is useful).}
}
\details{
	The first two functions allow the programmer to manually read
	the request input, if any. The next two functions employ the
	libapreq2 library to parse HTTP POST input into apr\_table objects. Both
	groups are mutually exclusive of each other, e.g. don't call \code{apache.get_post} 
	after calling \code{apache.read}, and vise versa.

	Currently, \code{apache.write} is the only way to write data
	to the client. If R's connection streams are made available to
	package writers in the future, then this could change and one
	could call cat(...,file=r) or something similar.

	The last two functions are independent of how input is consumed.
}
\value{
	Both \code{apache.read} and \code{apache.readline} return the
	next available input as a character vector of length 1, or NULL
	if there's no more input left to read or an error occurred.

	\code{apache.write} returns TRUE on success and FALSE on failure. (This may need to change)

	\code{apache.get_post} returns an object of type apr\_table corresponding to the HTTP POST data parsed.
	
	\code{apache.get_uploads} returns a list of the files uploaded via HTTP POST when the encoding type is set to \code{multipart/form-data}.
		
	\code{apache.get_args} returns an object of type apr\_table. Any data after the \code{?} in a URL or form data uploaded when method is set to GET is considered as \code{args} data.
		
	\code{apache.get_cookies} return objects of type apr\_table corresponding to the request header \code{Cookies}.
}
\section{Note on File Uploading}{
	\code{apache.get_uploads} returns a list of lists corresponding to each form element of type file. Consider the following HTML form: \preformatted{

<form enctype="multipart/form-data" method="POST" action="URL">
	<input type="file" name="FirstFile">
	<input type="file" name="SecondFile">
<input type="submit" name="Upload>
}
Then \code{apache.get_uploads} returns the following list: \preformatted{

$FirstFile
$FirstFile$name
[1] "filename"

$FirstFile$tmp_name
[1] "/tmp/apreqv5IVrs"


$SecondFile
$SecondFile$name
[1] "anotherfile"

$SecondFile$tmp_name
[1] "/tmp/apreq8WEhni"
}
The \code{name} element's value is the actual name of the file uploaded, and
the \code{tmp_name} element's value is an absolute path to the uploaded file and only exists for the lifetime of the request.
}
\keyword{programming}
