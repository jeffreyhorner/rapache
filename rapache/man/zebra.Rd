\name{zebra}
\alias{zebra}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
%%  ~~function to do ... ~~
}
\description{
%%  ~~ A concise (1-5 lines) description of what the function does. ~~
}
\usage{
zebra(title, l)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{title}{
%%     ~~Describe \code{title} here~~
}
  \item{l}{
%%     ~~Describe \code{l} here~~
}
}
\details{
%%  ~~ If necessary, more details than the description above ~~
}
\value{
%%  ~Describe the value returned
%%  If it is a LIST, use
%%  \item{comp1 }{Description of 'comp1'}
%%  \item{comp2 }{Description of 'comp2'}
%% ...
}
\references{
%% ~put references to the literature/web site here ~
}
\author{
%%  ~~who you are~~
}
\note{
%%  ~~further notes~~
}

%% ~Make other sections like Warning with \section{Warning }{....} ~

\seealso{
%% ~~objects to See Also as \code{\link{help}}, ~~~
}
\examples{
##---- Should be DIRECTLY executable !! ----
##-- ==>  Define data, use random,
##--	or do  help(data=index)  for the standard data sets.

## The function is currently defined as
function (title, l) 
{
    if (!is.null(title)) 
        cat("<h2><a name=\"", hrefify(title), "\"> </a>", title, 
            "</h2>", sep = "")
    cat("<table><tbody>", sep = "")
    n <- names(l)
    mapply(zebelem, if (is.null(n)) 
        rep(NA, length(l))
    else n, l)
    cat("</tbody></table>\n")
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
