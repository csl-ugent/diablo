/*!\addtogroup RELOCS Relocations 

-# Relocation(s) from a compiler/linker perspective
   \n\n
   Compilers do not know what the final layout of a program will be and thus
   cannot generate correct addresses when they create objectfiles. The real
   addresses can only be generated when the entire program is available (i.e.
   at link-time for static linked programs). So a linker will produce "bogus"
   addresses and depend on the linker to fix them up. The process of fixing up
   these addresses is called (static) relocation.
   \n\n
   As it is far from trivial to find addresses in compiled code, a special
   datastructure is added to the objectfiles, identifying those addresses that
   need to be adapted. This datastructure is called the relocation table and an
   element in this table is called a relocation. Apart from giving us the
   address where we need to relocate, a relocation also describes how the
   address at that place needs to be calculated.
   \n\n
   Every objectfile format has it's own representation for relocations, but
   typically a relocation contains the following information:
   - the place where we need to relocate (typically represented as a tuple
     <section,offset in the section>)  <A>
   - the object we are refering to with this relocation (typically a symbol
     (which is also a tuple <section,offset in the section>, see
     documentation/symbols_in_diablo)  <S>
   - an offset (this offset is either stored in the relocation itself (and then
     it is called an explicit addend) or it stored at the address we need to
     relocate (implicit addend))                <O>
   - a rule for the relocation         <R>
   .
   Typically the rules can express a complex statement like:
   \n\n
   *<A>=(*<A>0xff000000)|(0xffffff&((<S>+<O>-<A>)>>2))\n
   Which would write at <A> the upper to bits that already where at <A> and the
   relative offset from <S>+<O> to <A> shifted down 2 positions. (This actually
   is a very typical relocation for branches: the upper n bits represent an
   opcode, the lower an offset to jump to (which can be scaled if all
   instructions need to be aligned))
   \n\n
   Or something simple like:
   \n\n
   *<A>=<S>
   \n\n
   For completeness we need to add that a linker does not need to relocate all
   addresses in the program: a compiler can sometimes produce
   (inter-object-file) addresses on its own: if we want to produce an other
   value in the code section, we can do this by simply adding an offset to the
   program counter (or an other address???). Since the compiler will only move
   the entire section, it's not necessary to relocate these address
   calculations.
   \n\n
-# Relocations from Diablo's perspective
   \n\n
   -# Generic Relocation Encoding
      \n\n
      Relocations consist of a function, a relocatable object reference, an
      offset in the relocatable object and an addend.\n
      Relocation functions are encoded as follows:\n
      Bits 31 - 30 : (1,0,-1) * (relocatable object reference (begin or end) address / size + offset + addend)\n 
      Bits 29 - 28 : (1,0,-1) * (relocated address)\n 
      Bit  27 - 26 : (0=use begin address, 1=use end address, 2=use size)  
      Bit  25      : clear entire structure\n
      Bits 24 - 17 : (0,...,255) relocated size\n
      Bits 16 - 8  : (0,...,255) left shift\n
      Bits 7 -0    : (0,...,255) right shift\n 
   -# Relocations in object representation
      \n\n
      Like the linker, Diablo uses relocations to identify (and fix) addresses
      in the binary. To get the relocations, Diablo takes the map file, reads
      the original objects used during linking and merges all relocations in
      one table.  Relocations are are of the format:
      \n\n
      {\n
	  #t_reloc::reloc_type; // How to perform the relocation\n 
	  #t_reloc::from a_sub_section (stored in #t_reloc::from::section) + an offset (stored in offset);\n 
	  #t_reloc::to   a_symbol_to_a_subsection (stored in to) = <a_sub_section + an offset>;\n
      }\n\n
   -# Relocations in disassembled representation
      \n\n 
      When we disassemble a section, we no longer want the relocations to point
      to this section, but to an instruction. The same goes for relocations
      that really are from a relocations (e.g. the branch example). For
      relocations comimg from and going to instructions, the representation
      becomes:
      {\n
           #t_reloc::from a #t_ins\n 
	   #t_reloc::to   a #t_ins\n
      }\n\n
   -# Relocations in flowgraph representation
      \n\n
      When we go to a flowgraph representation, relocations no longer point to
      any type of instruction: they must point to the first instruction of a
      basic block, else the assumption that there is only one entrypoint to the
      bbl is violated. On the other hand, if we delete the first instruction in
      the basic block, we probably want the relocation to point to the second.
      Thus, we hav relocation to basic blocks. From instruction is still valid. 
*/
/*@{*/
/*@}*/
