# Output starts here
setContentType("text/html")

cat('<HTML><head><style type="text/css">\n') 
cat('table { border: 1px solid #8897be; border-spacing: 0px; font-size: 10pt; }')
cat('td { border-bottom:1px solid #d9d9d9; border-left:1px solid #d9d9d9; border-spacing: 0px; padding: 3px 8px; }')
cat('td.l { font-weight: bold; width: 10%; }\n')
cat('tr.e { background-color: #eeeeee; border-spacing: 0px; }\n')
cat('tr.o { background-color: #ffffff; border-spacing: 0px; }\n')
cat('</style></head><BODY><H1>receiveBin() test for RApache</H1>\n')
cat('<form enctype=multipart/form-data method=POST>\n',sep='')
cat('Enter a string: <input type=text name=name value=""><br>\n',sep='')
cat('Enter another string: <input type=text name=name value=""><br>\n',sep='')
cat('Upload a file: <input type=file name=fileUpload><br>\n')
cat('Upload another file: <input type=file name=anotherFile><br>\n')
cat('<input type=submit name=Submit>')

cat("<hr><pre>\n")
while(length(x <- receiveBin(30)) > 0){
    cat(' raw:', x,'\nchar:',sep=' ')
    for (i in 1:length(x)){
	if (x[i] == 0) cat(' \\0')
	else if (x[i] == 13) cat(' \\r')
	else if (x[i] == 10) cat(' \\n')
	else cat(' ',rawToChar(x[i]))
    }
    cat('\n')
}
cat("<pre>\n")
cat("SERVER INTERNALS\n")
print(list(readStarted=SERVER$internals('readStarted'),postParsed=SERVER$internals('postParsed')))
cat("SERVER\n")
print(SERVER)
cat("</pre>\n")
