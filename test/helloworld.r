setContentType("text/html")
setHeader("foo","bar")
setCookie("foo","bar",Sys.time()+1000)
cat("<html><body><h1>Hello World!</h1>")
cat("<pre>")
print(warnings())
cat("search:\n")
print(search())

cat("\n.GlobalEnv:\n")
print(ls(globalenv()))

assign("gfoo","gbar",.GlobalEnv)
if (exists("foo")){
	foo <- foo + 1
} else {
	foo <- 1
}
cat("foo is",foo,"\n<br>");

cat("\nls():\n")
print(ls())

if (!is.null(GET)){
	for (i in 1:length(GET) ){
		cat("\t",names(GET)[i],":",GET[[i]],"\n")
#		cat("\t",GET[[i]],"\n")
	}
} else {
	cat("\nGET is NULL.\n")
}

cat("</pre>")
cat("</body></html>")
DONE
