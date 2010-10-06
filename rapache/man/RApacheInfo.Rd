\name{RApacheInfo}
\alias{RApacheInfo}
%- Also NEED an '\alias' for EACH other topic documented here.
\title{
%%  ~~function to do ... ~~
}
\description{
%%  ~~ A concise (1-5 lines) description of what the function does. ~~
}
\usage{
RApacheInfo()
}
%- maybe also 'usage' for other objects documented here.
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
function () 
{
    cat("<html><head>")
    cat("<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />")
    cat("<style type=\"text/css\">")
    cat("body { font-family: \"lucida grande\",verdana,sans-serif; margin-left: 210px; margin-right: 18px; }")
    cat("table { border: 1px solid #8897be; border-spacing: 0px; font-size: 10pt; }")
    cat("td { border-bottom:1px solid #d9d9d9; border-left:1px solid #d9d9d9; border-spacing: 0px; padding: 3px 8px; }")
    cat("td.l { font-weight: bold; width: 10\%; }")
    cat("tr.e { background-color: #eeeeee; border-spacing: 0px; }")
    cat("tr.o { background-color: #ffffff; border-spacing: 0px; }")
    cat("div a { text-decoration: none; color: white; }")
    cat("a:hover { color: #8897be; background: white; }")
    cat("tr:hover { background: #8897be; }")
    cat("img.map { position: fixed; border: 0px; left: 50px; right: auto; top: 10px; }")
    cat("div.map { background: #8897be; font-weight: bold; color: white; position: fixed; bottom: 30px; height: auto; left: 15px; right: auto; top: 110px; width: 150px; padding: 0 13px; text-align: right; font-size: 12pt; }")
    cat("div.map p { font-size: 10pt; font-family: serif; font-style: italic; }")
    cat("div.h { font-size: 20pt; font-weight: bold; }")
    cat("h4 { font-size: 10pt; font-weight: bold; color: grey;}")
    cat("hr {background-color: #cccccc; border: 0px; height: 1px; color: #000000;}")
    cat("</style>")
    cat("<title>RApacheInfo()</title>")
    cat("<meta name=\"ROBOTS\" content=\"NOINDEX,NOFOLLOW,NOARCHIVE\" />")
    cat("</head>")
    cat("<body>")
    cat("<a name=\"Top\"> </a>")
    cat("<a href=\"http://www.r-project.org/\"><img class=\"map\" alt=\"R Language Home Page\" src=\"http://www.r-project.org/Rlogo.jpg\"/></a>")
    cat("<div class=\"h\">RApache version ???</div>")
    cat("<div class=\"map\">")
    cat("<p>jump to:</p>")
    cat("<a href=\"#Top\">Top</a><br/><hr/>")
    cat("<a href=\"#")
    cat(hrefify("R.version"))
    cat("\">R.version</a><br/>")
    cat("<a href=\"#")
    cat(hrefify("search()"))
    cat("\">search()</a><br/>")
    cat("<a href=\"#")
    cat(hrefify(".libPaths()"))
    cat("\">.libPaths()</a><br/>")
    cat("<a href=\"#")
    cat(hrefify("options()"))
    cat("\">options()</a><br/>")
    cat("<a href=\"#")
    cat(hrefify("Sys.getenv()"))
    cat("\">Sys.getenv()</a><br/>")
    cat("<a href=\"#")
    cat(hrefify("Sys.info()"))
    cat("\">Sys.info()</a><br/>")
    cat("<a href=\"#")
    cat(hrefify(".Machine"))
    cat("\">.Machine</a><br/>")
    cat("<a href=\"#")
    cat(hrefify(".Platform"))
    cat("\">.Platform</a><br/>")
    cat("<a href=\"#")
    cat(hrefify("Cstack_info()"))
    cat("\">Cstack_info()</a><br/><hr/>")
    cat("<a href=\"#Attached_Packages\">Attached Packages</a><br/><hr/>")
    cat("<a href=\"#Installed_Packages\">Installed Packages</a><br/><hr/>")
    cat("<a href=\"#License\">License</a><br/><hr/>")
    cat("<a href=\"#People\">People</a>")
    cat("</div>")
    zebra("R.version", R.version)
    cat("<br/><hr/>")
    zebra("search()", search())
    cat("<br/><hr/>")
    zebra(".libPaths()", .libPaths())
    cat("<br/><hr/>")
    zebra("options()", options())
    cat("<br/><hr/>")
    zebra("Sys.getenv()", as.list(Sys.getenv()))
    cat("<br/><hr/>")
    zebra("Sys.info()", as.list(Sys.info()))
    cat("<br/><hr/>")
    zebra(".Machine", .Machine)
    cat("<br/><hr/>")
    zebra(".Platform", .Platform)
    cat("<br/><hr/>")
    zebra("Cstack_info()", as.list(Cstack_info()))
    cat("<br/><hr/>")
    cat("<h1><a name=\"Attached_Packages\"></a>Attached Packages</h1>")
    lapply(sub("package:", "", search()[grep("package:", search())]), 
        zebrifyPackage)
    cat("<h1><a name=\"Installed_Packages\"></a>Installed Packages</h1>")
    lapply(attr(installed.packages(), "dimnames")[[1]], zebrifyPackage)
    cat("<h2><a name=\"License\"></a>License</h2>")
    cat("<pre>")
    cat("Copyright 2005  The Apache Software Foundation\n")
    cat("\n")
    cat("Licensed under the Apache License, Version 2.0 (the \"License\");\n")
    cat("you may not use this file except in compliance with the License.\n")
    cat("You may obtain a copy of the License at\n")
    cat("\n")
    cat("  <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">http://www.apache.org/licenses/LICENSE-2.0\"</a>\n")
    cat("\n")
    cat("Unless required by applicable law or agreed to in writing, software\n")
    cat("distributed under the License is distributed on an \"AS IS\" BASIS,\n")
    cat("WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n")
    cat("See the License for the specific language governing permissions and\n")
    cat("limitations under the License.\n")
    cat("</pre><hr/>\n")
    cat("<h2><a name=\"People\"></a>People</h2>\n")
    cat("<p>Thanks to the following people for their contributions, giving advice, noticing when things were broken and such. If I've forgotten to mention you, please email me.</p>\n")
    cat("<pre>\n")
    cat("\tGregoire Thomas\n")
    cat("\tJan de Leeuw\n")
    cat("\tKeven E. Thorpe\n")
    cat("\tJeremy Stephens\n")
    cat("\tAleksander Wawer\n")
    cat("\tDavid Konerding\n")
    cat("\tRobert Kofler\n")
    cat("\tJeroen Ooms\n")
    cat("\tMichael Driscoll\n")
    cat("</pre>")
    cat("</body></html>")
  }
}
% Add one or more standard keywords, see file 'KEYWORDS' in the
% R documentation directory.
\keyword{ ~kwd1 }
\keyword{ ~kwd2 }% __ONLY ONE__ keyword per line
