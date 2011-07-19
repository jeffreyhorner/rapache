user2007example <- function(p1=.95,p2=.7){
	x11(width=10,height=10)
	sc <- Weibull2(c(1,3),c(p1,p2))
	rcens <- function(n) 1 + (5-1) * (runif(n) ^ .5)
	f <- Quantile2(sc,
		hratio=function(x) ifelse(x <= .75, 1, .75),
		dropin=function(x) ifelse(x <= .5, 0, .15 * (x-.5)/(5-.5)),
		dropout=function(x) .3*x/5
	)
	par(mfrow=c(2,2))
	plot(f,'all',label.curves=list(keys='lines'))
}
