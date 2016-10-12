/* "7y^2-1!=x^2 */
/* Overflow elimination */
pushal
andl         $16383,%edx //0x3fff
andl         $65535,%ebx //0xffff
/* Opaque predicate*/
imull        %edx, %edx
leal         0(,%edx,8), %ecx
subl         %edx, %ecx
decl         %ecx
imull        %ebx, %ebx
cmpl         %ebx, %ecx
popal

