#ifndef BIN2VM_H
#define BIN2VM_H

#include <bin2vm/softvm/bin2vm_status_codes.h>

#ifndef BIN2VM_LINKAGE
  #ifdef WIN32
    #define BIN2VM_LINKAGE extern __declspec(dllimport)
  #else
    #define BIN2VM_LINKAGE
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#else //C
  #define bool int
  #define FALSE 0
  #define TRUE  !(FALSE)
#endif

struct Bin2Vm;
typedef struct Bin2Vm Bin2Vm;

struct bin2vm_list_vmimages_arm; //Forward declartion, include bin2vm_list_vmimages_arm.h for a full defintion.

/* Typedefs for bin2vm_create_iselect, bin2vm_check_arm_instruction and bin2vm_destroy that
   make the use of dlopen/dlsym easier. */
typedef enum bin2vm_status_codes (*bin2vm_create_iselect_t)( Bin2Vm** retSelf );
typedef enum bin2vm_status_codes (*bin2vm_check_arm_instruction_t)( Bin2Vm* self, unsigned char* armInstruction, unsigned int sizeArmInstruction, bool* retIsSupported );
typedef enum bin2vm_status_codes (*bin2vm_destroy_t)( Bin2Vm** pSelf );


BIN2VM_LINKAGE const char* bin2vm_getVersion(void);

/** Initialize the library in instruction selection mode.
    @param retSelf address of a Bin2Vm pointer variable. The function uses this pointer-pointer to return the created library handle.
    @return BIN2VM_STATUS_SUCCESS when successful or a different error code when an error occurred.
*/
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_create_iselect( Bin2Vm** retSelf );


/** Initialize the library in translation mode.
    @param retSelf address of a Bin2Vm pointer variable. The function uses this pointer-pointer to return the created library handle.
    @return BIN2VM_STATUS_SUCCESS when successful or a different error code when an error occurred.
*/
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_create( Bin2Vm** retSelf );


/** Release library resources.
    @param pSelf Address of the Bin2Vm pointer variable. After releasing the resources, the function will set *pSelf to NULL.
    @return BIN2VM_STATUS_SUCCESS when successful or a different error code when an error occurred.
*/
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_destroy( Bin2Vm** pSelf );


/** Check a given arm instruction.
     @param self the library handle.
     @param armInstruction a pointer to a buffer containing a ARM-instruction as machine code.
     @param sizeArmInstruction the size of the buffer, usually 4.
     @param retIsSupported a pointer to a bool (aka int) which the function uses to return its decision (true if the instruction is supported otherwise false.
     @return BIN2VM_STATUS_SUCCESS when successful or a different error code when an error occurred.
*/
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_check_arm_instruction( Bin2Vm* self, unsigned char* armInstruction, unsigned int sizeArmInstruction, bool* retIsSupported );


/** Post-Linker interface. This function gets a JSON buffer containing chunk to be translated as well as symbols with a specified address.
 *   Include bin2vm_list_vmimages_arm.h to process and 
 *   @param self the library handle.
 *   @param jsonBuffer a pointer to the buffer containg the JSON.
 *   @param sizeJsonBuffer the size of the JSON buffer.
 *   @param writeAsmFilename if not null: Instruct the bin2vm to also emit an asm-file. This is mainly used for testing.
 *   @param retVmImages used to return the list with the VM-Images. Use bin2vm_free_vmimages_arm to free the list.
 *   @return BIN2VM_STATUS_SUCCESS when successful or a different error code when an error occurred.
 */
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_diablo_phase2( Bin2Vm* self,
                                                              char* jsonBuffer, unsigned int sizeJsonBuffer,
                                                              const char* writeAsmFilename,
                                                              const char* dbgFilenameBody,
                                                              struct bin2vm_list_vmimages_arm** retVmImages
);

/** Free the list returned by bin2vm_diablo_phase2
 *    @param pList address of the struct bin2vm_list_vmimages_arm* variable containing the list.
 *    @return BIN2VM_STATUS_SUCCESS when successful or a different error code when an error occurred.
 */
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_free_vmimages_arm( struct bin2vm_list_vmimages_arm** pList );


/* @param: retVmImages: If not null: returns the translated vmImages. This is only used for testing.
 *                      Use bin2vm_free_vmimages_arm to free the list. The list is defined in bin2vm_list_vmimages_arm.h.
 */
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_diablo_phase1( Bin2Vm* self,
                                                              const char* readJsonFileName, const char* writeAsmFilename, bool ignoreAddresses,
                                                              struct bin2vm_list_vmimages_arm** retVmImages
);

BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_freeData( void** pData );

BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_translate_arm( Bin2Vm* self,
                                                              unsigned char* armBuffer, unsigned int sizeArmBuffer,
                                                              unsigned char** retVmImage, unsigned int* retSizeVmImage
);

/* Caller must free the returned image using bin2vm_freeData( void** pData ); */
BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_translate_arm_instruction( Bin2Vm* self,
                                                                          unsigned char* armBuffer, unsigned int sizeArmBuffer,
                                                                          unsigned char** retVmImage, unsigned int* retSizeVmImage
); //Used for testing individual instructions.


BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_translate( Bin2Vm* self,
                                                          unsigned char* x86buffer, unsigned int sizeX86buffer, unsigned int startAddress,
                                                          unsigned char** retVmImage, unsigned int* retSizeVmImage
);

BIN2VM_LINKAGE enum bin2vm_status_codes bin2vm_shutdown_llvm(); //Used to remove still-reachable blocks from valgrind output.

#ifdef __cplusplus
}
#endif

#endif /* BIN2VM_H */
