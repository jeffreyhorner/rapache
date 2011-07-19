foo <- 'bar'

main <- function(){
	setContentType('text/html')
	cat('<h1>Hello',foo,'!</h1>\n')
	foo <<- 'touched by main'
}

handler <- function(){
	setContentType('text/html')
	cat('<h1>Hello',foo,'!</h1>\n')
	foo <<- 'touched by handler'
}
