/* dub0) 2|x v 8|(x^2-1) {{{ */
/* Opaque predicate */
testl        $1, %eax
je           .Good //HAS TO GO TO THE CORRECT PART!!
pushl        %eax
pushl        %edx
/* Overflow elimination */
andl         $65535,%eax //0x0000ffff
imull        %eax, %eax
decl         %eax
andl         $7, %eax
popl         %edx
popl         %eax
