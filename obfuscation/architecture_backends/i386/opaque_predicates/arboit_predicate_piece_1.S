/* maintain program semantics */
pushal
/* input variable x (ebx) */
movl    %edx,%eax
andl    $65535,%eax //0xffff
movl    $5,%ebx
xorl    %edx,%edx
divl    %ebx
/* read the number out of the table */
movl    0(%ebx,%edx,4),%esi
