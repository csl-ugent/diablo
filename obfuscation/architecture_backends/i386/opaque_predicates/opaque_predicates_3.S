/* 2) 3|(x^3-x) {{{ */
/* Overflow elimination */
pushal
andl         $1023,%ecx //0x3ff
/* Opaque predicate*/
movl         %ecx, %eax
imull        %ecx, %eax
imull        %ecx, %eax
subl         %ecx, %eax
movl         $3, %edx
movl         %edx, %ecx
cltd                   //double word to quad word
idivl        %ecx      // eax= (edx:eax)/ecx edx=ramainder
testl        %edx, %edx
popal
