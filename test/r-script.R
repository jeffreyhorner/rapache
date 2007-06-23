fooey <- function(file,env){
	setContentType("text/plain")
	cat("File:",file,"contents:\n")
	source(file)
}
