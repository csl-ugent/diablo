/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* Symbol Typedefs {{{ */
#ifndef DIABLOOBJECT_SYMBOL_TYPEDEFS
#define DIABLOOBJECT_SYMBOL_TYPEDEFS
typedef struct _t_compressed_symbol t_compressed_symbol;
#endif /* }}} Symbol Typedefs */
/* Symbol Defines {{{ */
#ifndef DIABLOOBJECT_SYMBOL_DEFINES
#define DIABLOOBJECT_SYMBOL_DEFINES

#define ADDRESS_IS_CODE 1
#define ADDRESS_IS_DATA 2
#define ADDRESS_IS_SMALL_CODE 3

#define SYMBOL_TYPE_FILE                  0x1
#define SYMBOL_TYPE_FUNCTION              0x2
#define SYMBOL_TYPE_MARKER                0x4
#define SYMBOL_TYPE_TLS                   0x8
#define SYMBOL_TYPE_FUNCTION_SMALLCODE    0x10
#define SYMBOL_TYPE_OBJECT                0x20
#define SYMBOL_TYPE_NOTYPE                0x40
#define SYMBOL_TYPE_SECTION               0x80

/* Symbols of the $d.x $a.x $t.x type get marked with one of these flags. These
   flags will then later on be used to check instead of the actual '$d' strings. */
#define SYMBOL_TYPE_MARK_DATA             0x100
#define SYMBOL_TYPE_MARK_CODE             0x200
#define SYMBOL_TYPE_MARK_THUMB            0x400
/* symbol is exported from the binary and should be added as an entrypoint 
 * Additionally, .(r)data symbols that reference this symbol should be kept live during 
 * dead data elimination */
#define SYMBOL_TYPE_EXPORT                0x1000
/* symbol is imported by the binary. Calls that reference this symbol 
 * should have their outgoing edges to HELL rerouted to DYNCALL hell */
#define SYMBOL_TYPE_IMPORT                0x2000
#define SYMBOL_TYPE_DYNLINK_MASK          0x3000

/* symbol orders */
#define SYMBOL_ORDER_LOCAL    -1
#define SYMBOL_ORDER_WEAK     5
#define SYMBOL_ORDER_GLOBAL   10
#define SYMBOL_ORDER_OVERRIDE 12

#define T_SYMBOL(x) ((t_symbol *) x)
#define AddressNullForSymbolTable(st) AddressNullForObject(st->obj)
#define AddressNewForSymbolTable(st,a) AddressNewForObject(st->obj,a)

#define SymbolTableAddSymbol(symbol_table,name,code,order,dup,search,sec,offset,addend,tentative,size,flags)  realSymbolTableAddAliasedSymbol (__FILE__, __LINE__,symbol_table,name,name,code,order,dup,search,sec,offset,addend,tentative,size,flags)
#define SymbolTableAddAliasedSymbol(symbol_table,name,alias,code,order,dup,search,sec,offset,addend,tentative,size,flags)  realSymbolTableAddAliasedSymbol (__FILE__, __LINE__,symbol_table,name,alias,code,order,dup,search,sec,offset,addend,tentative,size,flags)

#define SymbolTableRemoveSymbol(symbol_table,symbol)	realSymbolTableRemoveSymbol (__FILE__, __LINE__, symbol_table, symbol)

/*
 * Diablo Symbol Tokens
 * These are used for parsing expressions in $diablo symbols.
 * Casts to avoid warning because used with t_ptr_array.
 */
#define DSTC (void*)(long)
#define DST_SYMBOL     0
#define DST_INTEGER    1
#define DST_OP         2
#define DST_LPAR       3
#define DST_RPAR       4

#endif /* }}} Symbol Defines */
#include <diabloobject.h>
#include <string.h>
#ifdef DIABLOOBJECT_TYPES
/* Symbol Types {{{ */
#ifndef DIABLOOBJECT_SYMBOL_TYPES
#define DIABLOOBJECT_SYMBOL_TYPES
struct _t_compressed_symbol
{
  t_uint32 base_idx;
  t_uint32 offset_from_start;
  t_uint32 addend;
  t_uint32 size;
  t_uint32 code;
  t_uint32 tentative;
  t_uint32 order_dup_search;
  t_uint32 name;
};
#endif

/*
 * These are used to access the tokenize table used in parsing $diablo symbols.
 */
#define dst_valid(dst_tab, c) \
 !( dst_tab[(unsigned char) (c)] & 0x1)

#define dst_symbolname(dst_tab, c) \
  ( dst_tab[(unsigned char) (c)] & 0x2)

#define dst_digit(dst_tab, c) \
  ( dst_tab[(unsigned char) (c)] & 0x4)

#define dst_arithmetic(dst_tab, c) \
  ( dst_tab[(unsigned char) (c)] & 0x8)

#define dst_whitespace(dst_tab, c) \
  !( dst_tab[(unsigned char) (c)] & 0xF)

#define dst_precedence(op) _dst_precedence((long)op)
#define _dst_precedence(op) \
    ((op == '(') ? 0 : (op == '+' || op == '-') ? 1 : 2)

/* }}} */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Symbol Functions {{{ */
#ifndef DIABLOOBJECT_SYMBOL_FUNCTIONS
#define DIABLOOBJECT_SYMBOL_FUNCTIONS
/*! Symbol table constructor */
t_symbol_table *SymbolTableNew (t_object *);

t_symbol *SymbolTableGetSymbolByName (const t_symbol_table *, t_const_string);
t_symbol * realSymbolTableAddAliasedSymbol (t_const_string file, int line, t_symbol_table * symbol_table, t_const_string name, t_const_string alias, t_const_string code, t_int32 order, t_tristate dup, t_tristate search, t_relocatable * sec, t_address offset, t_address addend, t_const_string tentative, t_address size, t_uint32 flags);
/* adds an absolute symbol at address 0 with name concat(prefx,symname) and the
 * specified flags; generally useful to pass information from the object parsing
 * to the linker script
 */
t_symbol *SymbolTableAddAbsPrefixedSymWithFlagsIfNonExisting(t_symbol_table *st, t_object *absobj, t_const_string prefix, t_const_string symname, t_uint32 flags);


void SymbolTableFree (const t_symbol_table *);
t_symbol *SymbolTableGetFirstSymbolWithName (const t_symbol_table *, t_const_string);
t_uint32 SymbolTableGetDataType (const t_object *, t_address);
t_uint32 SymbolTableGetCodeType (const t_object *, t_address);
t_symbol *SymbolTableGetSymbolByAddress (const t_symbol_table *, t_address);
t_hash_table *SymbolTableCreateHashTableForGetFirst(const t_symbol_table * st, t_const_string next_symbol_target);
t_hash_table *SymbolTableCreateHashTableForGetFirstLimitedToNamePattern(const t_symbol_table * st, t_const_string name_pattern);
t_symbol *SymbolTableGetFirstSymbolByAddress (const t_symbol_table *, t_address);
t_symbol *SymbolTableGetNextSymbolByAddress (const t_symbol *, t_address);
void SymbolTableSortSymbolsByAddress (t_symbol_table *, t_const_string);
void realSymbolTableRemoveSymbol (t_const_string file, int line, t_symbol_table * table, const t_symbol * sym);

void SymbolTableSetSymbolTypes (t_symbol_table *st);

t_symbol * SymbolTableDupSymbol(t_symbol_table * symbol_table, const t_symbol * in, t_const_string new_name);
t_symbol * SymbolTableDupSymbolWithOrder(t_symbol_table * symbol_table, const t_symbol * in, t_const_string new_name, t_int32 order);
void SymbolSetBase(t_symbol * sym, t_relocatable * base);
void RelocatableSortSymRefs(t_relocatable *r);

void CreateSymbolTranslationTable();
void AddTranslatedSymbolName(t_const_string orgname, t_const_string newname);
t_const_string GetTranslatedSymbolName(t_const_string name);
t_const_string GetReverseTranslatedSymbolName(t_const_string name);
void TryParseSymbolTranslation(t_const_string objectfilename);
void FiniSymbolTranslationTable();
int SymbolDiabloSymbolReadData(const t_symbol *sym, t_uint64 *data, t_string *expr);
t_const_string SymbolDiabloSymbolExprToRelocString(t_const_string expr, t_string *sa, t_string *sb);
#endif /* }}} Symbol Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
