if (is.null(FILES)) rnorm(10)
tmpfile <- NULL
if(!is.null(SERVER) && ("method" %in% names(SERVER)) && (SERVER$method == "POST") ){
	tmpfile <- tempfile()
	con <- file(tmpfile,open="w")
	writeLines(readLines(),con);
	close(con);
}
setContentType("text/html")
cat("</html><body>")
cat('<H1>Using R To Read Input</H1>\n')
if (!is.null(tmpfile)) cat('<H3>We read to this file:',tmpfile,'</H3>\n')
cat('<form enctype="multipart/form-data" method=POST action="/test/read.r">\n')
cat('Upload a file: <input type=file name="fileUpload"><br>\n')
cat('<input type=submit name=Submit>')
cat("</form></body></html>")
-2
