/* 5) 2|x+x {{{ */
/* Take a (maybe) life variable */
movl         %eax, %ecx
/* Overflow elimination */
andl         $2147483647,%ecx //0x7fffffff
/* Opaque predicate*/
addl         %ecx, %ecx
andl         $1, %ecx //addl defines also the Parity flag, but the parity flag is wrong when there is an overflow (out of experimental results)
