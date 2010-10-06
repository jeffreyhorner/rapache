\name{setCookie}
\alias{setCookie}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
%%  ~~function to do ... ~~
}
\description{
%%  ~~ A concise (1-5 lines) description of what the function does. ~~
}
\usage{
setCookie(name = NULL, value = "", expires = NULL, path = NULL, domain = NULL, ...)
}
%- maybe also 'usage' for other objects documented here.
\arguments{
  \item{name}{
%%     ~~Describe \code{name} here~~
}
  \item{value}{
%%     ~~Describe \code{value} here~~
}
  \item{expires}{
%%     ~~Describe \code{expires} here~~
}
  \item{path}{
%%     ~~Describe \code{path} here~~
}
  \item{domain}{
%%     ~~Describe \code{domain} here~~
}
  \item{\dots}{
%%     ~~Describe \code{\dots} here~~
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
function (name = NULL, value = "", expires = NULL, path = NULL, 
    domain = NULL, ...) 
{
    args <- list(...)
    therest <- ifelse(length(args) > 0, paste(paste(names(args), 
        args, sep = "="), collapse = ";"), "")
    .Call("RApache_setCookie", name, value, expires, path, domain, 
        therest)
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
