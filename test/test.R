# Test request handler
testsum <- 0
hello <- function(r){
	apache.write(r,"<h1>Hello World ",testsum,"</h1")
	testsum <<- testsum + 1
	OK
}
handler <- function(r)
{
    args <- apache.get_args(r)
    post <- apache.get_post(r)
    cookies <- apache.get_cookies(r)
    uploads <- apache.get_uploads(r)

    if(is.null(args$called))
        called <- 1
    else
        called <- as.integer(args$called) + 1

    apache.add_cookie(r,"called",called,expires=Sys.time()+100)

    apache.write(r,"<HTML><BODY><H1>Hello from mod_R</H1>\n")
    apache.write(r,"<form enctype=multipart/form-data method=POST action=\"/test/R?called=",called,"\">\n",sep="")
    apache.write(r,"Enter a string: <input type=text name=name value=\"",post$name,"\"><br>\n",sep="")
    apache.write(r,"Enter another string: <input type=text name=name value=\"",post$name,"\"><br>\n",sep="")
    apache.write(r,"Upload a file: <input type=file name=file><br>\n")
    apache.write(r,"<input type=submit name=Submit>")

    apache.write(r,"<hr>\n")
    apache.write(r,"<h2>Args variables</h2>\n")
    apache.write(r,as.html(args));
    apache.write(r,"<h2>Post variables</h2>\n")
    apache.write(r,as.html(post))
    apache.write(r,"<h2>Cookies</h2>\n")
    apache.write(r,as.html(cookies))
    apache.write(r,"<h2>File Uploads</h2>\n")
    apache.write(r,as.html(uploads))
    apache.write(r,"<h2>Request Record</h2>\n")
    apache.write(r,as.html(r))
	apache.write(r,"<h2>About R</h2>\n")
	apache.write(r,"R.version: ",as.html(as.list(unlist(R.version))),'<br>',sep="")
	apache.write(r,"Sys.info(): ",as.html(as.list(Sys.info())),'<br>',sep="")
    apache.write(r,"Sys.getenv(): ",as.html(as.list(Sys.getenv())),'<br>',sep="")
	apache.write(r,".Platform: ",as.html(.Platform),'<br>',sep='')
	apache.write(r,".Machine: ",as.html(.Machine),'<br>',sep='')
    apache.write(r,".libPaths(): ",as.html(.libPaths()),'<br>',sep="")
	apache.write(r,"</BODY></HTML>\n")
    OK
}
