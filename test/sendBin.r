setContentType("image/png")
t <- tempfile()
png(t,type="cairo")
plot(rnorm(10))
dev.off()
setHeader('Content-Length',file.info(t)$size)
sendBin(readBin(t,'raw',n=file.info(t)$size))
unlink(t)
DONE
