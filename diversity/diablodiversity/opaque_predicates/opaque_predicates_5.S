/* 4) 2|x+x {{{ */
/* Overflow elimination */
andl         $2147483647,%eax //0x7fffffff
/* Opaque predicate*/
addl         %eax, %eax
andl         $1, %eax //addl defines also the Parity flag, but the parity flag is wrong when there is an overflow (out of experimental results)
