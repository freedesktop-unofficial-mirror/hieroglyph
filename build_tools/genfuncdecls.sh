#! /bin/sh

[ $# -lt 1 ] && echo "$0: <file>" && exit

awk 'BEGIN{a=0;u=0;f="stdin";}u==0{if(FILENAME!="-"){f=FILENAME;};u=1;}a==1{printf("%s",$0);if($0~/\)/){printf(";\n");a=0;}else{printf("\n");}}a==0&&$0~/^(static|G_INLINE_FUNC) [^=]+$/&&$0!~/;/{printf("#line %d \"%s\"\n", NR, f);printf("%s",$0);if($0!~/\)/){a=1;printf("\n");}else{print ";"}}' $1

