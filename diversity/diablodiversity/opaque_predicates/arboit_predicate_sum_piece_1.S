pushal
/* input variable x (eax) */
movl    %eax,%ebx
andl    $255,%ebx //0xff
incl    %ebx
leal    -1(%ebx,%ebx,1),%edx
movl    $1,%ecx
movl    $3,%eax
cmpl    $3,%edx
jb      0
