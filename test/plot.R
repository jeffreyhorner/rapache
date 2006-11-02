library('Cairo')
rplot <- function(r)
{
	apache.set_content_type(r,"image/png")
	Cairo(file=stdout())
	plot(runif(100),rnorm(100),main="Random")
	dev.off()
	OK
}
