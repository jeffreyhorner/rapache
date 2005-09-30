\name{directives}
\alias{directives}
\title{RApache directives used in Apace config file}
\description{
	RApache directives are instructions on how to load and prepare
	the R interpreter when the server is started.
\preformatted{
LoadModule R_module /path/to/mod_R.so

<Location /URL>
	SetHandler r-handler
	Rsource /path/to/R/code.R
	RreqHandler function-name
	Rlibrary library-name
</Location>

<Directory /file/path>
	AddHandler r-handler .R
	Rsource /path/to/R/code.R
	RreqHandler function-name
	Rlibrary library-name
</Directory>
}
}
\details{
    LoadModule tells apache two things, first the name of the module
    structure to add to the module list, and second the shared library
    to load.

	SetHandler and AddHandler are Apache directives that specify
	which module handler will be used for matching URLs or files. In
	the case of RApache, this is always r-handler.

	\emph{Rsource} calls \code{\link{source}} on its argument, an absolute
	path to a file containing R code.

	\emph{RreqHandler} sets the function to call each time a new requests
	comes in.

	\emph{Rlibrary} calls \code{\link{library}} on its argument. This
	may not be useful as there are no way to pass any arguments to
	the call.  One can always add the \code{\link{library}} call
	withing the \emph{Rsource} file.
}
\keyword{programming}
\alias{RreqHandler}
\alias{Rsource}
\alias{Rlibrary}
\seealso{
	\item{\code{\link{intro}}}{Details about RApache package}
}
