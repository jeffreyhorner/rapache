setContentType("image/png")
t <- tempfile()
png(t,type="cairo")
plot(rnorm(10))
dev.off()
sendBin(readBin(t,'raw',n=file.info(t)$size))
unlink(t)
DONE
