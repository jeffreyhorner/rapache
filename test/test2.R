# Test request handler
library('GDD')
library('NRart')
handler <- function(r)
{
    step <- 2
    args <- apache.get_args(r)

    if(!is.null(args$t)){
        pstep <- as.integer(args$t)
        if (pstep > step && pstep < 121)
            step <- pstep;
    }

    apache.set_content_type(r,"image/png")

    GDD(ctx=apache.gdlib_ioctx(r),w=500,h=500,type="png")

    nr.movie(x^3 + .28 * tan(x + t) + cos(x + 2*t)*.3i - 0.7,
                't', seq(0, pi, length=121)[step],
                extent=1, steps=3, points=400,
                col=rainbow(256), zlim=c(-pi, pi))

    dev.off()
    OK
}
