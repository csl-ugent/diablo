/* 6) 2|x*x/2 Arboit {{{ */
/* store changed regs ATT: Check flags! */
pushl        %eax
pushl        %edx
/* Overflow elimination */
andl         $65535,%eax //0xffff
imull        %eax,%eax
shrl         $1,%eax
andl         $1,%eax
testl        %eax,%eax
popl         %edx
popl         %eax

