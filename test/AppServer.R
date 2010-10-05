#
# A simple system for supporting many web applications
# installed under BASEDIR.
#
# The apache config might look something like this:
# <Location /app>
# RFileHandler /var/www/rApache/AppServer.R::AppServer
# </Location>
#

BASEDIR <- '/var/www/rApache'

# The Appserver function searches the URI for a segment that matches
# a directory under BASEDIR. If it exists, and a file handler.R is present, it
# is sourced in a new environment and a function called handler is expected to exist.
# If so, it is called and the return value is returned from AppServer().

AppServer <- function(){

	setContentType('text/html')

	setwd(BASEDIR)

	#cat(SERVER$uri,file=stderr())

	# First split the uri by "/", first element will always be "".
	uri <- strsplit(SERVER$uri,"/")[[1]][-1]

	# bad uri
	if (length(uri) == 0){
		cat(paste("Empty URI",SERVER$uri),file=stderr())
	   	return(OK)
	}

	# Find the list of dirs; we call them apps.
	apps <- rownames(subset(file.info(dir()), isdir==TRUE))

	# No apps to consider
	if (length(apps) == 0){
		cat(paste("Empty List of Applications for URI:",SERVER$uri),file=stderr())
	   	return(OK)
	}

	# returns a vector of uri index positions where apps are found in uri.
	i <- match(apps,uri,nomatch=0)

	# First non-zero index position of i is our first app found
	appIndex <- which(i > 0)[1]

	# No app exists in uri
	if (is.na(appIndex)){
		cat(paste("No Applications for URI:",SERVER$uri,"Apps:",apps),file=stderr())
	   	return(OK)
	}

	app <- apps[appIndex]

	# Now we have our app, but does a "handler.R" exist?
	if (!file.exists(paste(app,"handler.R",sep='/'))){
		cat(paste("Missing handler.R for Application:",app,'(',dir(app),')'),file=stderr())
	   	return(OK)
	}

	uriIndex <- i[appIndex]

	# Yes, now set up an environment from which to operate and go for it!

	appURI <- paste("/",paste(uri[1:uriIndex],collapse='/'),sep='')
	appPathInfo <- sub(appURI,'',SERVER$uri)
	setwd(app)

	# Source the handler
	sys.source("handler.R",envir=environment(AppServer))

	# Was a handler function created?
	if (exists("handler",mode="function",envir=environment(AppServer))){
		return(handler(appURI,appPathInfo))
	}

	OK
}
