/* input variable y (ecx) */
movl    %ecx,%eax
andl    $65535,%eax //0xffff
/* eax=eax*eax  (b*b) */
imull   %eax,%eax
/* edx =0 */
xorl    %edx,%edx
/* ebx = 4k+3 */
leal    3(,%esi,4),%ebx
/* edx:eax / ebx */
divl    %ebx
leal    1(%esi),%eax
cmpl    $1,%eax
movl    %edx,%ecx
movl    %edx,%edi
movl    %esi,%eax
jbe     0
