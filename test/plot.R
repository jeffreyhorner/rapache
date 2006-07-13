library('GDD')
rplot <- function(r)
{
	apache.set_content_type(r,"image/png")
	GDD(ctx=apache.gdlib_ioctx(r),w=500,h=500,type="png")
	plot(runif(100),rnorm(100),main="Random")
	dev.off()
	OK
}
