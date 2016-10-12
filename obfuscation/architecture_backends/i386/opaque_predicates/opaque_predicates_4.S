/* 3) x^2>=0{{{ */
/* Overflow elimination */
andl         $65535,%eax //0xffff
/* Opaque predicate*/
imull        %eax, %eax
testl        $0, %edx
