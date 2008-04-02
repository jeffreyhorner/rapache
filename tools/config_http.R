options(warn=-1)
NextAvailablePort <- function(){
	start <- 8181
	while(TRUE){
		if (start >= 65536) return(0)
		con <- try(socketConnection(port=start),silent=TRUE)
		if (inherits(con,'try-error')){
			return(start)
		}
		close(con)
		start <- start + 1
	}
}

#
# Variables that will get replaced in httpd.conf.in
#
DOCROOT <- paste(getwd(),'/test',sep='')
PORT <- NextAvailablePort()
BREWINSTALLED <- 'brew' %in% .packages(all.available=TRUE)

unlink('test/httpd.conf')

con <- file('test/httpd.conf',open='w+')
lines <- readLines('test/httpd.conf.in')
if (BREWINSTALLED){
	lines <- append(lines,c(
		'<Directory @DOCROOT@/brew>\n',
		'  SetHandler r-script\n',
		'  RHandler brew::brew\n',
		'</Directory>\n') )
}
lines <- gsub('@PORT@',PORT,lines)
lines <- gsub('@DOCROOT@',DOCROOT,lines)
writeLines(lines,con)
close(con)
