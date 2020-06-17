/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>

/* For documentation of the machine stack language type ./scripts/stackdoc in
 * the main diablo directory! */

/*#define DEBUG_STACK_EXEC */
#ifdef DEBUG_STACK_EXEC
#define DEBUG_STACK(m) DEBUG(m)
#else
#define DEBUG_STACK(m)
#endif

/* {{{ defines and typedefs */
#define GetImm64(s) GetImm(s,16)
#define GetImm32(s) (t_uint32) GetImm(s,8);
#define GetImm16(s) (t_uint16) GetImm(s,4);
#define GetImm8(s) (t_uint8) GetImm(s,2);

/*! This structure holds the stack, and the current position in the stack while
 * executing */
typedef struct _t_exec_stack
{
  t_address *stack;
  t_uint32 maxelem;
  t_int32 top;
} t_exec_stack;

/* }}} */

/* {{{ helper functions */
/* get immediate of given size from string s.
 * all immediates are unsigned hex numbers */
static t_uint64
GetImm (t_const_string s, int numchars)
{
  int i;
  t_uint64 ret = 0;

  for (i = 0; i < numchars; i++)
  {
    ret <<= 4;
    if ((*s >= '0') && (*s <= '9'))
      ret += (*s - '0');
    else if ((*s >= 'a') && (*s <= 'f'))
      ret += (*s - 'a' + 10);
    else if ((*s >= 'A') && (*s <= 'F'))
      ret += (*s - 'A' + 10);
    else
      FATAL(("Malformed number!"));

    s++;
  }
  return ret;
}

static void
StackInit (t_exec_stack * st)
{
  st->stack = Malloc (20 * sizeof (t_address));
  st->top = -1;
  st->maxelem = 20;
}

static void
StackFini (t_exec_stack * st)
{
  DEBUG_STACK(("Run done"));
  Free (st->stack);
  st->maxelem = 0;
  st->top = -1;
}

static void
Push (t_exec_stack * st, t_address elem)
{
  DEBUG_STACK(("push @G", elem));
  if (++st->top == st->maxelem)
  {
    /* enlarge stack */
    st->stack =
      Realloc (st->stack, (st->maxelem + 10) * sizeof (t_address));
    st->maxelem += 10;
  }
  st->stack[st->top] = elem;
}

static t_address
Pop (t_exec_stack * st)
{
  if (st->top < 0)
    FATAL(("Popping empty stack"));
  DEBUG_STACK(("pop @G", st->stack[st->top]));
  return st->stack[st->top--];
}

/* rotate the stack left: top becomes bottom */
static void
Rotl (t_exec_stack * st)
{
  t_address tmp;

  if (st->top == -1)
    FATAL(("Rotl on empty stack"));
  if (st->top == 0)
    return;

  tmp = st->stack[st->top];
  memmove (st->stack + 1, st->stack, st->top * sizeof (t_address));
  st->stack[0] = tmp;
}

/* rotate the stack right: bottom becomes top */
static void
Rotr (t_exec_stack * st)
{
  t_address tmp;

  if (st->top == -1)
    FATAL(("Rotr on empty stack"));
  if (st->top == 0)
    return;

  tmp = st->stack[0];
  memmove (st->stack, st->stack + 1, st->top * sizeof (t_address));
  st->stack[st->top] = tmp;
}

/* }}} */

static t_const_string skip_if_true_part(t_const_string program);
static t_const_string skip_if_false_part(t_const_string program);

static t_const_string skip_if_true_part(t_const_string program)
{
  /* program points to the character _after_ '?'
   * returns a pointer to the character _after_ ':' or '!' */
  t_const_string c = program;
  while (*c)
  {
    if (*c == ':' || *c == '!')
      return c+1;
    
    if (*c == '?')
    {
      /* nested if */
      c = skip_if_true_part(c+1);
      if (*(c-1) == ':')
        c = skip_if_false_part(c);
      /* both skip_if_true_part() and skip_if_false_part() return c+1,
       * so avoid errors in case we have "!!", since there's another ++c
       * below.
       */
      --c;
    }

    ++c;
  }
  return NULL;
}
static t_const_string skip_if_false_part(t_const_string program)
{
  /* program points to the character _after_ ':'
   * returns a pointer to the character _after_ '!' */
  t_const_string c = program;
  while (*c)
  {
    if (*c == '!')
      return c+1;
    
    if (*c == '?')
    {
      /* nested if */
      c = skip_if_true_part(c+1);
      if (*(c-1) == ':')
        c = skip_if_false_part(c);
      /* both skip_if_true_part() and skip_if_false_part() return c+1,
       * so avoid errors in case we have "!!", since there's another ++c
       * below.
       */
      --c;
    }

    ++c;
  }
  return NULL;
}

/* A constant variant of the StackExec function that is guaranteed not to make
 * any changes to the object. It will only calculate the result of a code and
 * return it.
 */
t_address
StackExecConst (t_const_string program, const t_reloc* reloc, const t_symbol* sym, t_uint32 mode, const t_object* obj)
{
  return StackExec (program, reloc, sym, NULL, FALSE, mode, (t_object*) obj);
}

/* {{{ the actual stack machine */
t_address
StackExec (t_const_string program, const t_reloc * rel, const t_symbol * sym, char *base, t_bool write, t_uint32 mode, t_object * obj)
{
  t_const_string cmd = program;
  t_exec_stack estack;
  t_exec_stack *st = &estack;
  t_address ret;
  t_address tmp, oper;
  t_address null;

  enum
  { CALC, WRITE, VERIFY } phase;

  int currloadins = 0, currstoreins = 0;

  char *bundleptr = base;
  char *memptr = base;
  
  DEBUG_STACK(("\n"));
  if (rel) DEBUG_STACK(("Running: %s (@G)", program, AddressAdd (SECTION_CADDRESS (T_SECTION (RELOC_FROM (rel))), RELOC_FROM_OFFSET (rel))));
  else     DEBUG_STACK(("Running: %s", program));  

  StackInit (st);


  /* adjust the memptr to point to the first reloced byte.
   * this allows us to keep the base pointer as simple as
   * sec->data or something like that */
  
  if (rel)
  {
   
    null = AddressSub (RELOC_FROM_OFFSET(rel), RELOC_FROM_OFFSET(rel));
    memptr = AddressAddDispl (memptr, RELOC_FROM_OFFSET(rel));
    /* For IA64 Bundles: */
    bundleptr = AddressAddDispl (bundleptr, AddressInverseMaskUint32 (RELOC_FROM_OFFSET(rel), 0xf));
    currloadins = AddressExtractUint32 (RELOC_FROM_OFFSET(rel)) & 0xf;
    currstoreins = AddressExtractUint32 (RELOC_FROM_OFFSET(rel)) & 0xf;
  }
  else if (sym) /* Symbols do not write (nor read)! */
  {
    null = AddressSub (SYMBOL_OFFSET_FROM_START(sym), SYMBOL_OFFSET_FROM_START(sym));
    memptr = NULL;
    bundleptr = NULL;
  }
  else 
    FATAL(("You need to supply either a relocation or a symbol to StackExec!"));
   
  ret = null;

  phase = CALC;

  while (*cmd)
  {
    DEBUG_STACK(("COMMAND %c", *cmd));
    switch (*(cmd++))
    {
      /* ignore spaces for readability */
      case ' ':
        break;

        /******** push commands ********/

      case 'u': /* Symbol is  (U)ndefined */
	{
		FATAL(("Implement u command"));
	}
      case 'U': /* Relocatable is the (U)ndefined section */
	{
		t_uint32 index;
		t_relocatable * relocatable=NULL;
		t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
		ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("U operand without suffix in %s!", program));
		index = 10 * ((*(cmd)) - '0');
		cmd++;
		ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("U operand without suffix in %s!", program));
		index += ((*(cmd)) - '0');
		cmd++;
                if (sym) 
                {
                  ASSERT(index == 0, ("Symbols currently can have only one relocatable (SYMBOL_BASE)"));
                  relocatable=SYMBOL_BASE(sym);
                  ASSERT(relocatable, ("Reloc program %s for symbol @S has no relocatable", program,sym));
                }
                else
                {
                  ASSERT(index < RELOC_N_TO_RELOCATABLES(rel), ("Use of non-existing relocatable in relocation @R: %s",rel,RELOC_CODE(rel)));
                  relocatable=RELOC_TO_RELOCATABLE(rel)[index];
                }

		if (relocatable==T_RELOCATABLE(undef))
		{
			Push (st, AddressAddUint32(null,1));
		}
		else
		{
			Push (st, null);
		}
	 
		break;
	}
      case 'M': /* (M)odus : pops an address of the stack and check if it refers to thumb code. If so 1 is pushed, else 0 is pushed */
        {
          t_address oper1 = Pop (st);

          if ((RELOC_N_TO_RELOCATABLES(rel)>0) &&
	      ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL) || 
	      (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_INS)))
          {
            t_address ret;

            if (DiabloBrokerCall("DeflowgraphedModus", &oper1, rel, &ret))
            {
               Push (st, ret);
            }
            else
            {
              Push (st, AddressNewForObject (obj, 0));
            }
          }
          else
          {
            if (ADDRESS_IS_SMALL_CODE ==
                SymbolTableGetCodeType (obj, oper1)
                && ADDRESS_IS_DATA != SymbolTableGetDataType (obj, oper1))
            {
              Push (st, AddressNewForObject (obj, 1));
            }
            else
            {
              Push (st, AddressNewForObject (obj, 0));
            }
          }
        }
        break;
      case 'm': /* potential (m)odus remainder of "Sxxm|" code */
        {
          /* Always return 0, this m will be changed to an M once we know it
           * refers to a code symbol
           */
          t_address oper1 = Pop (st);
          Push (st, AddressNewForObject (obj, 0));
        }
        break;
      case 'S': /* refered (S)ymbol - push the destination of the reloc. The name refers to the S used in the Elf manuals. */
        {
          t_relocatable * relocatable=NULL;
          t_uint32 index = 0;

	  ASSERT(!sym, ("The S operand can currently not be used to define symbols. Symbols should only use relocatables.")); 
	  
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated S operand without suffix in %s!", program));
	  index = 10 * ((*(cmd)) - '0');
	  cmd++;
	  ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated S operand without suffix in %s!", program));
          index += ((*(cmd)) - '0');
          cmd++;

          ASSERT(index < RELOC_N_TO_SYMBOLS(rel), ("index is too high"));

          t_symbol* symbol = RELOC_TO_SYMBOL(rel)[index];
          relocatable = SYMBOL_BASE(symbol);
	  ASSERT(relocatable, ("Reloc program %s for reloc @R has no relocatable", program,rel));

          {
            t_address symoffs;
            t_address a;

            symoffs = SYMBOL_OFFSET_FROM_START(symbol);
            if (SYMBOL_FLAGS(symbol) & (SYMBOL_TYPE_FUNCTION|SYMBOL_TYPE_FUNCTION_SMALLCODE))
              symoffs = AddressInverseMaskUint32(symoffs,OBJECT_CODE_SYMBOL_ADDRESS_INVERSE_MASK(obj));
            a = AddressAdd(RELOC_TO_SYMBOL_OFFSET(rel)[index],
                                     AddressAdd(RELOCATABLE_CADDRESS(relocatable),
                                                symoffs));
            Push(st, a);
          }
        }
        break;
      case 'O': /* the (O)ldest - pushes the address of the relocatables parent if it exists, else the address of the relocatable */
        {
          t_relocatable * relocatable=NULL;
          t_uint32 index = 0;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated S operand without suffix in %s!", program));
          index = 10 * ((*(cmd)) - '0');
          cmd++;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated S operand without suffix in %s!", program));
          index += ((*(cmd)) - '0');
          cmd++;
          if (RELOCATABLE_RELOCATABLE_TYPE(relocatable) == RT_SECTION)
          {
            if (mode == 1)
              Push (st, RELOCATABLE_MIN_ADDRESS(relocatable));
            else if (mode == 2)
              Push (st, RELOCATABLE_OLD_ADDRESS(relocatable));
            else
              Push (st, RELOCATABLE_CADDRESS(relocatable));
            break;
          }
          else if (RELOCATABLE_RELOCATABLE_TYPE(relocatable) == RT_SUBSECTION)
          {
            if (!SECTION_PARENT_SECTION(T_SECTION(relocatable)))
              FATAL(("Subsection has no parent in O-stack command"));
            if (mode == 1)
              Push (st,
                    SECTION_MIN_ADDRESS(SECTION_PARENT_SECTION
                                        (T_SECTION(relocatable))));
            else if (mode == 2)
              Push (st,
                    SECTION_OLD_ADDRESS(SECTION_PARENT_SECTION
                                        (T_SECTION(relocatable))));
            else
              Push (st,
                    SECTION_CADDRESS(SECTION_PARENT_SECTION
                                     (T_SECTION(relocatable))));
            break;
          }
          else
            FATAL(("Strange relocatable in O-stack command"));
        }
      case 'A': /* (A)ddend - push the addend of the reloc */
	{
          t_address addend;
          t_uint32 index = 0;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated A operand without suffix in %s!", program));
          index = 10 * ((*(cmd)) - '0');
          cmd++;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated A operand without suffix in %s!", program));
          index += ((*(cmd)) - '0');
          cmd++;

	  if (sym) 
	  {
            ASSERT(index == 0, ("Symbols currently can have only one addend (SYMBOL_ADDEND)"));
            addend=SYMBOL_ADDEND(sym);
          }
          else
          {
            ASSERT(index < RELOC_N_ADDENDS(rel), ("Use of non-existing addend (%u) in relocation @R: %s",index, rel,RELOC_CODE(rel)));
	    addend=RELOC_ADDENDS(rel)[index];
          }

          Push (st, addend);
	}
        break;
      case 'P': /* (P)osition - push the from address of the reloc */
        if (rel)
        {
          if (mode == 1)
          {
            Push (st, AddressAdd (RELOCATABLE_MIN_ADDRESS(RELOC_FROM(rel)), RELOC_FROM_OFFSET(rel)));
            DEBUG_STACK(("P @G", AddressAdd (RELOCATABLE_MIN_ADDRESS(RELOC_FROM(rel)), RELOC_FROM_OFFSET(rel))));
          }
          else if (mode == 2)
          {
            Push (st, AddressAdd (RELOCATABLE_OLD_ADDRESS(RELOC_FROM(rel)), RELOC_FROM_OFFSET(rel)));
            DEBUG_STACK(("P @G", AddressAdd (RELOCATABLE_OLD_ADDRESS(RELOC_FROM(rel)), RELOC_FROM_OFFSET(rel))));
          }
          else
          {
            Push (st, AddressAdd (RELOCATABLE_CADDRESS(RELOC_FROM(rel)), RELOC_FROM_OFFSET(rel)));
            DEBUG_STACK(("P @G", AddressAdd (RELOCATABLE_OLD_ADDRESS(RELOC_FROM(rel)), RELOC_FROM_OFFSET(rel))));
          }
        }
        else 
          FATAL(("You cannot use the P operand in symbol stack programs (only in reloc stack programs)"));
        break;
      case 'R': /* associated (R)elocatable - push the address of the associated relocatable (e.g. GOT section) */
        {
          t_relocatable * relocatable=NULL;
          t_address to_offset;
          t_uint32 index = 0;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated R operand without suffix in %s!", program));
          index = 10 * ((*(cmd)) - '0');
          cmd++;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated R operand without suffix in %s!", program));
          index += ((*(cmd)) - '0');
          cmd++;

	  if (sym) 
	  {
            ASSERT(index == 0, ("Symbols currently can have only one relocatable (SYMBOL_BASE)"));
            relocatable=SYMBOL_BASE(sym);
            to_offset=SYMBOL_OFFSET_FROM_START(sym);
            ASSERT(relocatable, ("Reloc program %s for symbol @S has no relocatable", program,sym));
          }
          else
          {
            ASSERT(index < RELOC_N_TO_RELOCATABLES(rel), ("Use of non-existing relocatable in relocation @R: %s",rel,RELOC_CODE(rel)));
	    relocatable=RELOC_TO_RELOCATABLE(rel)[index];
	    to_offset=RELOC_TO_RELOCATABLE_OFFSET(rel)[index];
          }

          if (mode == 1)
            Push (st,
                  AddressAdd (RELOCATABLE_MIN_ADDRESS(relocatable),
                              to_offset));
          else if (mode == 2)
            Push (st,
                  AddressAdd (RELOCATABLE_OLD_ADDRESS(relocatable),
                              to_offset));
          else
            Push (st,
                  AddressAdd (RELOCATABLE_CADDRESS(relocatable),
                              to_offset));
          break;

        }
      case 't': /* push the (t)o offset of the relocatable */
        FATAL(("Implement"));
        
        //Push (st, rel->to_offset);
        break;

      case 'i': /* (i)mmediate - push a 32-bit immediate */
        {
          t_uint32 imm = GetImm32 (cmd);

          tmp = AddressNewForObject (obj, imm);
          Push (st, tmp);
          cmd += 8; /* skip over the immediate */
        }
        break;
      case 'I': /* (I)mmediate - push a 64-bit immediate */
        {
          tmp = AddressNew64 (GetImm64 (cmd));
          Push (st, tmp);
          cmd += 16; /* skip over the immediate */
        }
        break;
      case 's': /* (s)hort - push a 16-bit unsigned immediate */
        {
          t_uint16 imm = GetImm16 (cmd);

          cmd += 4; /* skip over the immediate */
          tmp =
            AddressAddUint32 (null,
                              (t_uint32) imm);
          Push (st, tmp);
        }
        break;
      case 'Z': /* si(Z)e of destination - push the size of the reloc's destination */
        {
          t_relocatable * relocatable=NULL;
          t_uint32 index = 0;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated Z operand without suffix in %s!", program));
          index = 10 * ((*(cmd)) - '0');
          cmd++;
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), ("Depredecated Z operand without suffix in %s!", program));
          index += ((*(cmd)) - '0');
          cmd++;

	  if (sym) 
	  {
            ASSERT(index == 0, ("Symbols currently can have only one relocatable (SYMBOL_BASE)"));
            relocatable=SYMBOL_BASE(sym);
            ASSERT(relocatable, ("Reloc program %s for symbol @S has no relocatable", program,sym));
          }
          else
          {
            ASSERT(index < RELOC_N_TO_RELOCATABLES(rel), ("Use of non-existing relocatable in relocation @R: %s",rel,RELOC_CODE(rel)));
	    relocatable=RELOC_TO_RELOCATABLE(rel)[index];
          }

          if (mode == 1)
            Push (st, RELOCATABLE_MIN_SIZE(relocatable));
          else if (mode == 2)
            Push (st, RELOCATABLE_OLD_SIZE(relocatable));
          else
            Push (st, RELOCATABLE_CSIZE(relocatable));
          break;
        }
        /***** Memory operations *****/
      case 'l': /* (l)oad - load 32 bits and push */
        {
          t_uint32 data;
          memcpy(&data, memptr, sizeof(data));
          if (OBJECT_SWITCHED_ENDIAN(obj))
          {
            data = Uint32SwapEndian(data);
          }
          memptr += 4;
          tmp = AddressNewForObject (obj, data);
          Push (st, tmp);
        }
        break;

      case 'L': /* (L)oad - load 64 bits and push */
        {
          t_uint64 data;
          memcpy(&data, memptr, sizeof(data));
          memptr += 8;
          tmp = AddressNewForObject (obj, data);
          Push (st, tmp);
        }
        break;

      case 'E': /* Load and (E)xtend - load 32 bits and extend to 64 bit */
        {
          FATAL (("this opcode is now subsumed in 'l'"));
        }
        break;

      case 'g': /* Push starting address of got table: value of (g)p */
        {
          t_symbol *sym = (OBJECT_SUB_SYMBOL_TABLE(obj))?SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                                                               "_GLOBAL_OFFSET_TABLE_"):NULL;
          t_section *sec = SectionGetFromObjectByName (obj, ".got");

          if ((!sym) && (!sec))
            FATAL(("Got relocation found but no .got table section found!"));

          if (!sym)
          {
            if (mode == 1)
              Push (st, SECTION_MIN_ADDRESS(sec));
            else if (mode == 2)
              Push (st, SECTION_OLD_ADDRESS(sec));
            else
              Push (st, SECTION_CADDRESS(sec));
          }
          else
          {
            if (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SUBSECTION
                || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SECTION)
              sec = T_SECTION(SYMBOL_BASE(sym));

            if (mode == 1)
              Push (st,
                    AddressAdd (AddressSub
                                (StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj),
                                 SECTION_CADDRESS(sec)),
                                SECTION_MIN_ADDRESS(sec)));
            else if (mode == 2)
              Push (st,
                    AddressAdd (AddressSub
                                (StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj),
                                 SECTION_CADDRESS(sec)),
                                SECTION_OLD_ADDRESS(sec)));
            else
              Push (st,
                    AddressAdd (AddressSub
                                (StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj),
                                 SECTION_CADDRESS(sec)),
                                SECTION_CADDRESS(sec)));

          }

          break;
        }

      case 'e': /* (e)ncode as LEB128 */
        {
          int index = 0;
          t_uint32 memidx = 0;
          t_uint32 data, orig_data;

          /* Unlike for the 'w' option, this does not return to 4 bytes before in the section where it will write,  
             (i.e., no memptr -= 4), the reason being that there is no corresponding predecessing 'l' that always 
             reads a fixed number of 4 bytes before. */

          /* expected one number: length of LEB128 */
          ASSERT((((*(cmd)) >= '0') && ((*(cmd)) <= '9')), (" operand without suffix in %s!", program));
          index = (*(cmd)) - '0';
          cmd++;


          /* top of stack = 32-bit integer to be encoded */
          data = AddressExtractUint32(Pop(st));
          orig_data = data;

          /* encode as LEB128 */
          int i;
          for (i = 0; i < index; i++)
          {
            char encoded = data & 0x7f;
            if (i < index-1)
              encoded |= 0x80;
            memptr[memidx] = (char) encoded;
            memidx++;
            data >>= 7;
          }
          ASSERT(data == 0, ("could not write 0x%x as LEB128 with length %d @R", orig_data, index, rel));
          break;
        }

      case 'w': /* (w)rite - pop and store top of stack as 32 bits. Mind that this can also be used to store a 64 bit generic as 32 bit */
        {
          t_uint32 data = AddressExtractUint32 (Pop (st));

          if (OBJECT_SWITCHED_ENDIAN(obj))
          {
            data = Uint32SwapEndian(data);
          }

          /* first adjust memptr */
          memptr -= 4;
          memcpy(memptr, &data, sizeof(data));
        }
        break;

      case 'W': /* (W)rite - pop and store 64 bits */
        {
          t_uint64 data = G_T_UINT64 (Pop (st));

          /* first adjust memptr */
          memptr -= 8;
          memcpy(memptr, &data, sizeof(data));
        }
        break;

      case 'k': /* push si(k)steen to 32 - load 16 bits unsigned and push */
        {
          t_uint16 data;
          memcpy(&data, memptr, sizeof(data));

          /* advance mem pointer */
          memptr += 2;

          tmp = AddressNewForObject (obj, (t_uint32) data);
          Push (st, tmp);
        }
        break;

      case 'K': /* push si(K)steen to 64 - load 16 bits unsigned and push in 64-bit generic */
        {
          FATAL (("operation now subsumed in 'k'"));
        }
        break;

      case 'v': /* ???? - pop and write 16 bits */
        {
          t_uint16 data = AddressExtractUint32 (Pop (st));

          /* first adjust memptr */
          memptr -= 2;

          memcpy(memptr, &data, sizeof(data));
        }
        break;

        /******** operators ********/
      case '+': /* pop the two top elements off the stack, add them and push the result */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressAdd (oper1, oper2));
          DEBUG_STACK(("@G + @G = @G", oper1, oper2, AddressAdd (oper1, oper2)));
        }
        break;

      case '/': /* pop the two top elements off the stack, divide them and push the result */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressDivUint32 (oper1, AddressExtractUint32(oper2)));
          DEBUG_STACK(("@G / @G = @G", oper1, oper2, AddressAdd (oper1, oper2)));
        }
        break;

      case '?': /* pop the top element and skip until after ':', or '!' if zero */
        {
          t_address oper1;
          oper1 = Pop (st);
          if (AddressIsNull (oper1))
          {
            cmd = skip_if_true_part(cmd);
            if (!*cmd)
              FATAL(("Illegal if-then-else!"));
          }
        }
        break;
      case ':': /* else part of if-then-else, if we reach it, skip until after '!' */
        {
          cmd = skip_if_false_part(cmd);
          if (!*cmd)
            FATAL(("Illegal if-then-else!"));
        }
        break;
      case '!': /* endif part of an if-then-else. Needed to know where it ends */
        break;
      case '-': /* pop the two top elements off the stack, sub them and push the result */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressSub (oper1, oper2));
          DEBUG_STACK(("@G - @G = @G", oper1, oper2, AddressSub (oper1, oper2)));
        }
        break;
      case '_': /* reverse SUB pop the two top elements off the stack, sub them and push the result */
        {
          t_address oper1, oper2;

          oper1 = Pop (st);
          oper2 = Pop (st);
          Push (st, AddressSub (oper1, oper2));
          DEBUG_STACK(("@G - @G = @G", oper1, oper2, AddressSub (oper1, oper2)));
        }
        break;
      case '&': /* pop the two top elements off the stack, and them (bitwise) and push the result */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressAnd (oper1, oper2));
          DEBUG_STACK(("@G & @G = @G", oper1, oper2, AddressAnd (oper1, oper2)));
        }
        break;
      case '|': /* pop the two top elements off the stack, or them (bitwise) and push the result */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressOr (oper1, oper2));
        }
        break;
      case '^': /* pop the two top elements off the stack, xor them (bitwise) and push the result */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressXor (oper1, oper2));
        }
        break;
      case '~': /* pop the top element off the stack, negate it (bitwise) and push the result */
        {
          t_address oper1;

          oper1 = Pop (st);
          Push (st, AddressNot (oper1));
        }
        break;
      case '<': /* pop the two top elements off the stack, left shift the first by the last and push the result */
        /* shift left */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressShl (oper1, oper2));
          DEBUG_STACK(("@G < @G = @G", oper1, oper2, AddressShl (oper1, oper2)));
        }
        break;
      case '>': /* pop the two top elements off the stack, right shift the first by the last and push the result */
        /* shift right */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, AddressShr (oper1, oper2));
          DEBUG_STACK(("@G > @G = @G", oper1, oper2, AddressShr (oper1, oper2)));
        }
        break;
      case '#': /* sign extend */
        /* the word size to which the value is extended is
         * determined by the word size of the t_address on
         * the stack. */
        {
          t_uint32 signbit = GetImm8 (cmd);

          oper = Pop (st);
          cmd += 2; /* skip over immediate operand */
          oper = AddressSignExtend (oper, signbit);
          Push (st, oper);
        }
        break;
      case '@': /* pop and add to memptr */
        {
          t_int32 offset = AddressExtractInt32 (Pop (st));

          memptr += offset;
        }
        break;

        /******** side effects ********/
        /* empty */

        /****** stack manipulation *****/
      case '=': /* duplicate the top entry */
        {
          tmp = Pop (st);
          Push (st, tmp);
          Push (st, tmp);
        }
        break;
      case '%': /* swap the top elements of the stack */
        {
          t_address oper1, oper2;

          oper2 = Pop (st);
          oper1 = Pop (st);
          Push (st, oper2);
          Push (st, oper1);
        }
        break;
      case '{': /* rotate left: put top of stack at bottom of stack */
        Rotl (st);
        break;
      case '}': /* rotate right: put bottom of stack at top of stack */
        Rotr (st);
        break;
      case '*': /* delete the top element of the stack */
        Pop (st);
        break;
      case '\\':
        if (phase == CALC)
        {
          if (write)
          {
            /* Do nothing */
          }
          else
          {
            ret = Pop (st);
            ASSERT(st->top == -1, ("End of program but stack not empty!"));
            DEBUG_STACK(("return @G", ret));
            StackFini (st);
            return ret;
          }
          phase = WRITE;
        }
        else if (phase == WRITE)
        {
          phase = VERIFY;
        }
        else
        {
          FATAL(("malformed program %s", program));
        }
        break;
      case '$': /* end of input: pop the stack and return */
        ret = Pop (st);

        if (rel)
          ASSERT(st->top == -1, ("End of program but stack not empty (%d elements left)!\nprogram = %s\nrel = @R", st->top + 1, program, rel));
        else
          ASSERT(st->top == -1, ("End of program but stack not empty (%d elements left)!\nprogram = %s\nsym = @S", st->top + 1, program, sym));

        StackFini (st);
        return ret;
        break;
      case 'G': /* (G)lobal Offset Entry - pseudo opcode, gets expanded to the got entry of a symbol */
        FATAL(("Encountered pseudo opcode G while executing %s. This opcode refers to the got entry of the symbol, and should have been converted to a real relocatable.", program));

      case 'p': /* f(p)tr64lsb - pseudo opcode, gets expanded */
        FATAL(("Encountered pseudo opcode p while executing %s. This opcode should have been converted to a real relocatable.", program));
      case 'q': /* (q)uality relocation: LTOFF_FPTR22 - pseudo opcode, gets expanded */
        FATAL(("Encountered pseudo opcode q while executing %s. This opcode should have been converted to a real relocatable.", program));

      case 'N': /* check the (N)ame of the specified destination object type (needs a space after the name) */
        {
          t_relocatable *reloc;
          t_string end;/* Use non-const alias to temporarily change cmd */

          ASSERT (RELOC_N_TO_RELOCATABLES (rel) == 1, ("Expected a single target relocatable in relocation:\n@R", rel));
          reloc = RELOC_TO_RELOCATABLE (rel)[0];
          for (end = (t_string) cmd+1; *end != ' ' && *end != '\0'; end++)
            ;
          switch (*cmd++)
          {
            case 's':
              if (RELOCATABLE_RELOCATABLE_TYPE (reloc) == RT_SECTION ||
                  RELOCATABLE_RELOCATABLE_TYPE (reloc) == RT_SUBSECTION)
              {
                t_section *sec = T_SECTION(reloc);
                char old;
                old = *end;
                *end = '\0';
                if (!StringCmp (cmd, SECTION_NAME(sec)))
                {
                  Push (st, AddressNewForObject (obj, 1));
                  DEBUG_STACK (("Ns%s : 1", cmd));
                }
                else
                {
                  Push (st, AddressNewForObject (obj, 0));
                  DEBUG_STACK (("Ns%s : 0 (%s)", cmd, SECTION_NAME (sec)));
                }
                *end = old;
                cmd = end;
              }
              else
              {
                Push (st, AddressNewForObject (obj, 0));
                DEBUG_STACK (("Ns%s : 0 (%s)", cmd, SECTION_NAME (T_SECTION (reloc))));
              }
              break;
            default:
              FATAL(("Unknown object type '%c'", *cmd));
              break;
          }
          cmd = end;
        }
        break;

      case 'f': /* (f)ollow a chain of (2) relocations if the current is pointing to the following-up section */
        {
          /* I've only seen this as a compiler optimization in ppc64, where the
           * chain
           *            .text ---> .opd ---> .text 
           * is optimized as
           *            .text ---> .text
           *
           * This will operand will rewrite the relocation to point directly to
           * the final destination.
           */
          ASSERT (RELOC_N_TO_RELOCATABLES (rel) == 1, ("Expected a single target relocatable in relocation\n@R", rel));
          {
            t_address addr_chain, addr_to;
            //t_section *sec;
            t_reloc *rel_chain;
            t_reloc_ref *rel_ref;

            addr_chain = Pop (st);

            rel_chain = NULL;
            for (rel_ref = RELOCATABLE_REFERS_TO(RELOC_TO_RELOCATABLE(rel)[0]) ; rel_ref ; rel_ref = RELOC_REF_NEXT(rel_ref))
            {
              if (AddressIsEq (addr_chain,
                               AddressAdd (RELOCATABLE_CADDRESS (RELOC_FROM (RELOC_REF_RELOC(rel_ref))),
                                           RELOC_FROM_OFFSET (RELOC_REF_RELOC(rel_ref)))
                               ))
              {
                rel_chain = RELOC_REF_RELOC(rel_ref);
                break;
              }
            }
            ASSERT(rel_chain, ("No chained relocation from @R", rel));
            addr_to = StackExecConst (RELOC_CODE (rel_chain), rel_chain, NULL, 0, obj);
            Push(st, addr_to);
          }
        }
        break;

      case 'd': /* Symbol is  (d)efined */
        {
          t_symbol *s = NULL;

          if (!sym)
          {
            s = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE (obj), RELOC_LABEL (rel));
          }

          if (!s)
          { /* Symbol is defined in another object */
            Push (st, AddressNewForObject (obj, 1));
            DEBUG_STACK(("d 1"));
          }
          else if (SYMBOL_ORDER (s) == 4 && SYMBOL_DUP (s) == PERHAPS && SYMBOL_SEARCH (s) == FALSE)
          { /* WEAK UNDEFINED symbol */
            Push (st, AddressNewForObject (obj, 0));
            DEBUG_STACK(("d 0 (weak undefined)"));
          }
          else
          {
            Push (st, AddressNewForObject (obj, 1));
            DEBUG_STACK(("d 1"));
          }
        }
        break;

      default:
        FATAL(("Unknown stack machine opcode %c (in %s at %s)", *(cmd - 1),
               program, cmd));

    }
  }
  /* shouldn't get here */
  if (sym)
    FATAL(("Improperly terminated program:\nprogram=\"%s\"\nfor symbol @S", program,sym));
  else
    FATAL(("Improperly terminated program:\n%s", program));
  /* keep the compiler happy */
  StackFini (st);
}

/* }}} */

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
