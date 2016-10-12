/* 1) 2|x+x^2 {{{ */
/* Overflow elimination */
andl         $32767,%eax //0x7fff
/* Opaque predicate*/
movl         %eax, %ecx
imull        %eax, %ecx
addl         %ecx, %eax
andl         $1, %eax
