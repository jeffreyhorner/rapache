cat('options()$warn is',getOption('warn'),'\n')
for (i in 1:100) warning("warning! you have been warned!")
cat('warnings:\n')
print(warnings())
