\name{ralite}
\alias{ralite}
\title{A simple robust iterative web server for R.
}
\description{
Provides a web server and development environment for R.
}
\usage{
ralite(host = "127.0.0.1", port = 8181, handler = function(req) {
    setContentType("text/html")
    cat("<h1>Hello World!</h1>")
})
}
\arguments{
  \item{host}{ IP address or hostname.
  }
  \item{port}{ Port number on which to accept connections.
  }
  \item{handler}{
		A function with one argument to handle each web request.
  }
}
\details{
	Provides a web development environment.
}
\value{
	no value
}
\author{
Jeffrey Horner
}
\keyword{http}
