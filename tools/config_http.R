APXS <- commandArgs(trailingOnly=TRUE)[1]
HTTPD <- commandArgs(trailingOnly=TRUE)[2]
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

unlink('test/confs/load_modules')
# Test if mime module compiled into httpd
if (length(grep('mime',readLines(pipe(paste(HTTPD,'-l')))))==0){
	# No, we need to add it. grab LIBEXECDIR
	con <- file('test/confs/load_modules',open='w+')
	libexecdir <- readLines(pipe(paste(APXS,'-q LIBEXECDIR')))[1]
	cat('LoadModule mime_module ',libexecdir,'/mod_mime.so\n',sep='',file=con)
	close(con)
}
