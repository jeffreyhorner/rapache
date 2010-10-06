ralite <- function(host='127.0.0.1',port=8181,handler=function(req){setContentType("text/html"); cat("<h1>Hello World!</h1>"); }) { 
	.Call(runRALITE,host,port,handler)
}
