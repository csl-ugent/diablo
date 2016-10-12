/* dub1) 3|(7x-5) => 9|(28x^2-13x-5) {{{ */
/* Set the input variable */
movl         %ebx, %eax
/* Overflow elimination */
/* Opaque predicate */
shll         $3,%eax
subl         %edx,%eax
subl         $5,%eax
movl         $3,%edx
movl         %edx,%ecx
movl         $0,%edx
idivl        %ecx
testl        %edx, %edx
je           .Good
movl    %ebx,%eax
negl    %eax
subl    $3,%eax
movl    $9,%edx
xorl    %eax,%ecx
movl    %edx,%ebx
movl    %ecx,%eax
xorl    %edx,%edx
divl    %ebx
movl   %edx,%eax
