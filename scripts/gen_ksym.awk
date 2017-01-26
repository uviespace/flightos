#!/usr/bin/awk  -f
BEGIN{ print "#include <stddef.h>\n #include \"include/kernel/ksym.h\""; print "struct ksym __ksyms[]={" }
{ if(NF==3){print "{\"" $3 "\", (void*) 0x" $1 "},"}}
END{print "{NULL,NULL} };"}
