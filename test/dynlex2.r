setContentType("text/html")
 listEnclosures <- function(env=NULL){
	empty <- environmentName(emptyenv())
	if (!is.null(env)) e <- env		
	else e <- parent.frame();
	n <- environmentName(e);
	i <- 0
	while (n != empty){
		if (n == '') n <- attributes(e)$name[1]
		cat('ENV:',i,n,'------------------------------\n');
		vars <- ls(envir=e)
		if (length(vars) > 10)
			cat('\t',vars[1:10],'....\n')
		else
			cat('\t',vars,'\n')

		e <- parent.env(e)
		n <- environmentName(e)
		i <- i + 1
	}
}

listFrames <- function(){
	f <- sys.frames();
	len <- length(f) - 1 ;
	if (len  < 1) return(invisible());
	cat("in listFrames\n")	
	for ( i in len:1 ){
		cat('FRAME:',len-i,'----------------\n')
		vars <- ls(envir=f[[i]])
		if (length(vars) > 10)
			cat('\t',vars[1:10],'....\n')
		else
			cat('\t',vars,'\n')
	}
}
cat("<HTML> <BODY> <H1>Call Stack</H1> <pre>")
listFrames()
cat("</pre> <H1>Enclosures</H1> <pre>")
listEnclosures()
cat("</pre> ")
cat("did we change it?",DONE,"<br>")
cat("returning",OK,"</BODY> </HTML>")
OK
