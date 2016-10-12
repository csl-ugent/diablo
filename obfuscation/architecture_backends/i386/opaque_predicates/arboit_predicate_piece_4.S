movl    %edi,%eax
xorl    %edx,%edx
divl    %ebx
movl    %edx,%esi
imull   %edx,%esi
subl    %ecx,%esi
movl    %esi,%eax
xorl    %edx,%edx
divl    %ebx
movl    %edx,%eax
testl   %edx, %edx
popal
jnz     0
