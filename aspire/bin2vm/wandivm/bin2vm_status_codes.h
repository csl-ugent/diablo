#ifndef BIN2VM_STATUS_CODES_H
#define BIN2VM_STATUS_CODES_H

enum bin2vm_status_codes
{
  BIN2VM_STATUS_SUCCESS                                = 0,
  BIN2VM_STATUS_NO_MEMORY                              = 1,
  BIN2VM_STATUS_USAGE_ERROR                            = 2,
  BIN2VM_STATUS_INPUT_CODE_NOT_SUPPORTED               = 3,
  BIN2VM_STATUS_INTERNAL_ERROR                         = 4,
  BIN2VM_STATUS_CTE_ERROR                              = 5,
  BIN2VM_STATUS_YAJL_ERROR                             = 6,
  BIN2VM_STATUS_BUFFER_TOO_SMALL                       = 7,
  BIN2VM_STATUS_PARSER_UNEXPECTED_INPUT                = 8,
  BIN2VM_STATUS_ARM_DISSASSEMBLE_ERROR                 = 9,
  BIN2VM_STATUS_PARSER_DESERIALIZE_ERROR               = 10,
  BIN2VM_STATUS_NOT_FOUND                              = 11,
  BIN2VM_STATUS_NOT_IMPLEMENTED                        = 12,
  BIN2VM_STATUS_IO_ERROR                               = 13,
  BIN2VM_STATUS_ASSERT_ERROR                           = 14,
  BIN2VM_STATUS_TRANSLATION_ERROR                      = 15,
  BIN2VM_STATUS_TRANSLATION_CONTROLFLOW_WRONG_NOTATION = 16,
  BIN2VM_STATUS_PARSER_MISSING_INPUT                   = 17
};

enum bin2vm_vm_type
{
  BIN2VM_VM_TYPE_NOTYPE    = 0,
  BIN2VM_VM_TYPE_CLASSIC   = 1, //Target softvm (static stack machine).
  BIN2VM_VM_TYPE_NEXTGEN   = 2, //Target wandivm (lli based).
  BIN2VM_VM_TYPE_NEXTGEN2  = 3, //Custom VM.
  BIN2VM_VM_TYPE_GENERATED = 4  //Diversified Custom VM.
};

#endif /* BIN2VM_STATUS_CODES_H */
