/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Objects */
#define OBJECT_FOREACH_SUBOBJECT(obj,sub_obj,tmp) for(tmp=OBJECT_MAPPED_FIRST(obj); tmp!=NULL; tmp=OBJECT_NEXT(tmp)) for (sub_obj=tmp; sub_obj!=NULL; sub_obj=OBJECT_EQUAL(sub_obj))

#define SECTIONARRAYPOINTERBYNUMBER(obj,number) (((number & 0x1f00000)==0x000000)?&(OBJECT_CODE(obj)): \
                                                 (((number & 0x1f00000)==0x100000)?&(OBJECT_DATA(obj)): \
                                                  (((number & 0x1f00000)==0x200000)?&(OBJECT_RODATA(obj)): \
                                                   (((number & 0x1f00000)==0x300000)?&(OBJECT_NOTE(obj)): \
                                                    (((number & 0x1f00000)==0x400000)?&(OBJECT_BSS(obj)): \
                                                     (((number & 0x1f00000)==0x500000)?&(OBJECT_TLSDATA(obj)): \
                                                      (((number & 0x1f00000)==0x600000)?&(OBJECT_TLSBSS(obj)): \
                                                       (((number & 0x1f00000)==0x700000)?&(OBJECT_DEBUG(obj)): \
                                                        (((number & 0x1f00000)==0x800000)?&(OBJECT_ATTRIB(obj)): \
                                                         NULL)))))))))

#define SECTIONARRAYBYNUMBER(obj,number) (((number & 0x1f00000)==0x0)?OBJECT_CODE(obj): \
                                          (((number & 0x1f00000)==0x100000)?OBJECT_DATA(obj): \
                                           (((number & 0x1f00000)==0x200000)?OBJECT_RODATA(obj): \
                                            (((number & 0x1f00000)==0x300000)?OBJECT_NOTE(obj): \
                                             (((number & 0x1f00000)==0x400000)?OBJECT_BSS(obj): \
                                              (((number & 0x1f00000)==0x500000)?OBJECT_TLSDATA(obj): \
                                               (((number & 0x1f00000)==0x600000)?OBJECT_TLSBSS(obj): \
                                                (((number & 0x1f00000)==0x700000)?OBJECT_DEBUG(obj): \
                                                 (((number & 0x1f00000)==0x800000)?OBJECT_ATTRIB(obj): \
                                                  NULL)))))))))

#define SECTIONCOUNTPOINTERBYNUMBER(obj,number) (((number & 0x1f00000)==0x0)?&(OBJECT_NCODES(obj)): \
                                                 (((number & 0x1f00000)==0x100000)?&(OBJECT_NDATAS(obj)): \
                                                  (((number & 0x1f00000)==0x200000)?&(OBJECT_NRODATAS(obj)): \
                                                   (((number & 0x1f00000)==0x300000)?&(OBJECT_NNOTES(obj)): \
                                                    (((number & 0x1f00000)==0x400000)?&(OBJECT_NBSSS(obj)): \
                                                     (((number & 0x1f00000)==0x500000)?&(OBJECT_NTLSDATAS(obj)): \
                                                      (((number & 0x1f00000)==0x600000)?&(OBJECT_NTLSBSSS(obj)): \
                                                       (((number & 0x1f00000)==0x700000)?&OBJECT_NDEBUGS(obj): \
                                                        (((number & 0x1f00000)==0x800000)?&OBJECT_NATTRIBS(obj): \
                                                         NULL)))))))))

#define SECTIONCOUNTBYNUMBER(obj,number) (((number & 0x1f00000)==0x0)?OBJECT_NCODES(obj): \
                                          (((number & 0x1f00000)==0x100000)?OBJECT_NDATAS(obj): \
                                           (((number & 0x1f00000)==0x200000)?OBJECT_NRODATAS(obj): \
                                            (((number & 0x1f00000)==0x300000)?OBJECT_NNOTES(obj): \
                                             (((number & 0x1f00000)==0x400000)?OBJECT_NBSSS(obj): \
                                              (((number & 0x1f00000)==0x500000)?OBJECT_NTLSDATAS(obj): \
                                               (((number & 0x1f00000)==0x600000)?OBJECT_NTLSBSSS(obj): \
                                                (((number & 0x1f00000)==0x700000)?OBJECT_NDEBUGS(obj):\
                                                 (((number & 0x1f00000)==0x800000)?OBJECT_NATTRIBS(obj): \
                                                  0)))))))))

#define OBJECT_FOREACH_SECTION(obj, sec, tel) for (tel=0; (tel&0x1f00000)<0x900000; tel+=0x100000, tel&=~0xfffff) for (sec=SECTIONARRAYBYNUMBER(obj,tel)?SECTIONARRAYBYNUMBER(obj,tel)[0]:NULL; ((tel&0xfffff)<SECTIONCOUNTBYNUMBER(obj,tel)); tel++, sec=((tel&0xfffff)<SECTIONCOUNTBYNUMBER(obj,tel))?SECTIONARRAYBYNUMBER(obj,tel)[tel&0xfffff]:NULL)

/* blatant abuse of the for statement, but it is the concisest way of doing this */
#define OBJECT_FOREACH_CODE_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NCODES(obj) && ((sec = OBJECT_CODE(obj)[tel]) || 1); tel++)
#define OBJECT_FOREACH_RODATA_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NRODATAS(obj) && ((sec = OBJECT_RODATA(obj)[tel])|| 1); tel++)
#define OBJECT_FOREACH_DATA_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NDATAS(obj) && ((sec = OBJECT_DATA(obj)[tel]) || 1); tel++)
#define OBJECT_FOREACH_BSS_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NBSSS(obj) && ((sec = OBJECT_BSS(obj)[tel]) || 1); tel++)
#define OBJECT_FOREACH_NOTE_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NNOTES(obj) && ((sec = OBJECT_NOTE(obj)[tel]) || 1); tel++)
#define OBJECT_FOREACH_DEBUG_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NDEBUGS(obj) && ((sec = OBJECT_DEBUG(obj)[tel]) || 1); tel++)
#define OBJECT_FOREACH_ATTRIB_SECTION(obj,sec,tel) for (tel=0; tel<OBJECT_NATTRIBS(obj) && ((sec = OBJECT_ATTRIB(obj)[tel]) || 1); tel++)

/* Sections */
#define SECTION_FOREACH_SUBSECTION(sec,sub) for (sub = SECTION_SUBSEC_FIRST(sec); sub; sub = SECTION_SUBSEC_NEXT(sub))
#define SECTION_FOREACH_SUBSECTION_SAFE(sec,sub,safe) for (sub = SECTION_SUBSEC_FIRST (sec), safe = sub ? SECTION_SUBSEC_NEXT (sub) : NULL; sub; sub = safe, safe = sub ? SECTION_SUBSEC_NEXT (sub) : NULL)

/* Relocs */

#define OBJECT_FOREACH_RELOC(object,relptr) for (relptr=RELOC_TABLE_FIRST(OBJECT_RELOC_TABLE(object)); relptr!=NULL; relptr=RELOC_NEXT(relptr))
#define OBJECT_FOREACH_RELOC_R(object,relptr) for (relptr=RELOC_TABLE_LAST(OBJECT_RELOC_TABLE(object)); relptr!=NULL; relptr=RELOC_PREV(relptr))
#define OBJECT_FOREACH_RELOC_SAFE(object,relptr,tmpptr) for (relptr=RELOC_TABLE_FIRST(OBJECT_RELOC_TABLE(object)),tmpptr=relptr?RELOC_NEXT(relptr):NULL; relptr!=NULL; relptr=tmpptr,tmpptr=relptr?RELOC_NEXT(relptr):NULL)

/* Symbols */
#define OBJECT_FOREACH_SYMBOL(object,symptr) for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(object)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
#define SYMBOL_TABLE_FOREACH_SYMBOL(symboltable,symptr) for (symptr=SYMBOL_TABLE_FIRST(symboltable); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
#define OBJECT_FOREACH_SYMBOL_SAFE(object,symptr,tmpptr) for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(object)),tmpptr=symptr?SYMBOL_NEXT(symptr):NULL; symptr!=NULL; symptr=tmpptr,tmpptr=symptr?SYMBOL_NEXT(symptr):NULL)
#define SYMBOL_TABLE_FOREACH_SYMBOL_SAFE(symboltable,symptr,tmpptr) for (symptr=SYMBOL_TABLE_FIRST(symboltable),tmpptr=symptr?SYMBOL_NEXT(symptr):NULL; symptr!=NULL; symptr=tmpptr,tmpptr=symptr?SYMBOL_NEXT(symptr):NULL)

/* vim: set shiftwidth=2 expandtab foldmethod=marker tw=80 */
