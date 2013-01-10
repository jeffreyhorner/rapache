handler <- function() {
   setContentType("text/plain")
   setStatus(400L)
   cat("Hi, I'm Ale!\n")
   OK
}
