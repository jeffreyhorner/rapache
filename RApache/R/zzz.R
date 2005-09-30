.First.lib <- function(lib, pkg)
{
    if (commandArgs()[1] != "mod_R")
        warning("\nRApache is only useful within the Apache web server.\nIt is loaded now for informational purposes only\n\n")
	else 
		library.dynam(pkg)
}
