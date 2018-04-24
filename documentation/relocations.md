# Relocations
Relocations are handled by a stack machine. The code for a stack machine
consists of three parts:

1.  The computation part
2.  The write part
3.  The check part

These parts are separated by '\\\\'. A stack program is terminated by a
'\$' character. The stack should be empty after this command.

~~~~
$    end of input: pop the stack and return 
~~~~

## The Computation Part

The computation part should leave exactly one value on the stack: the
value that needs to be computed. This part of the relocation program
should be executable on its own, without the write and check parts. The
computation can be specified in the stack language. The most important
commands are listed below. A more extensive explanation can be found by
executing the `scripts/stackdoc` script found in your diablo
distribution.

### Pushing addresses of relocatables:

~~~~
P    push the address+offset of the from relocatable
A    push the addend of the reloc

R00  push the address of the to relocatable
Z00  push the size of the to relocatable
t    push the offset of the to relocatable

R01  push the address of the extra relocatable
Z01  push the size of the extra relocatable
r    push the offset of the extra relocatable
~~~~
         
### Pushing immediates

~~~~
s   push a 16-bit unsigned immediate 
i   push a 32-bit immediate 
I   push a 64-bit immediate 
~~~~
         
### Operations on the stack and its contents

~~~~
=   duplicate the top entry
*   delete the top element of the stack
%   swap the tow top elements of the stack
{   rotate left: put top of stack at bottom of stack 
}   rotate right: put bottom of stack at top of stack 

~   pop the top element off the stack, negate it (bitwise) and push the result 
+   pop the two top elements off the stack, add them and push the result
-   pop the two top elements off the stack, sub them and push the result
<   pop the two top elements off the stack, left shift the first by the last and push the result 
>   pop the two top elements off the stack, right shift the first by the last and push the result
&   pop the two top elements off the stack, and them (bitwise) and push the result
|   pop the two top elements off the stack, or them (bitwise) and push the result 
^   pop the two top elements off the stack, xor them (bitwise) and push the result 
~~~~

### if-then-else

~~~~
?   pop the top element and skip until after first ':', or '!' if zero 
:   else part of if-then-else, if we reach it, skip until after '!' 
!   endif part of an if-then-else. Needed to know where it ends 
~~~~
         
## The write part

In the write part, you can use the operations described above. A write
part usually ends with one of the following:

~~~~
v   pop and write 16 bits 
w   pop and write 32 bits
W   pop and write 64 bits 
~~~~

## The check part

In the check part, you can use the operations as described above. After
the check, the value on top of the stack should be zero, if not diablo
will halt in error.
