handler <- function(){
   loc <- '/index.html'
   if (!is.null(GET) && !is.null(GET$loc) && GET$loc != '')
      loc <- GET$loc
   setHeader("Location",loc)
   return(HTTP_MOVED_TEMPORARILY)
}
