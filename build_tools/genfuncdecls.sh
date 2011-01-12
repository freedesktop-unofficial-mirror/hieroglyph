#! /bin/sh

[ $# -lt 1 ] && echo "$0: <file>" && exit

awk 'BEGIN{a=0;u=0;f="stdin";}u==0{if(FILENAME!="-"){f=FILENAME;};u=1;}a==1{print $0}a==1&&$0~/\)/{printf(";\n");a=0;}a==0&&$0~/^(static|G_INLINE_FUNC) [^=]+$/&&$0!~/;/{printf("#line %d \"%s\"\n", NR, f);print $0;if($0!~/\)/){a=1;}else{print ";"}}' $1

