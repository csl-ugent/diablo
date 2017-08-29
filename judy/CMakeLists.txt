set(bitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(bitness 64)
endif()
  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

if (${bitness} EQUAL 64)
  add_definitions(-DJU_64BIT)
endif()

# Disable the -Warray-bounds warning for Judy. There is a hack present in judy/JudyCommon/JudyGet.c
# that uses the CHECKINDEXNATIVE macro on an array with a size too small for the requested index. This
# hack works however as there is a second array present behind the first array with a large enough size.
# (See jpi_t in judy/JudyCommon/JudyPrivateBranch.h)
if (HAS_WARNING_ARRAY_BOUNDS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-array-bounds")
endif()
# Disable aggressive loop optimizations for Judy. The hack described here above uses undefined behavior
# which the compiler might optimize erroneously.
if (HAS_FLAG_AGGRESSIVE_LOOP_OPTIMIZATIONS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-aggressive-loop-optimizations")
endif()

macro(judygen name define output)
  add_executable(${name} JudyCommon/JudyTables.c)
  target_compile_definitions(${name}
    PRIVATE ${define}
  )
  target_include_directories(${name}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/JudyCommon
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Judy1
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/JudyL
  )

  add_custom_command(
    OUTPUT  ${output}
    DEPENDS ${name}
    COMMAND ./${name}
    COMMENT "Generating Judy tables for ${define}")
endmacro()

judygen(Judy1TablesGen JUDY1 Judy1Tables.c)
judygen(JudyLTablesGen JUDYL JudyLTables.c)

# HATE HATE HATE JUDY HATE
function(judy_prevnext judy)
  set(prev ${CMAKE_CURRENT_BINARY_DIR}/${judy}_Prev.c)
  set(next ${CMAKE_CURRENT_BINARY_DIR}/${judy}_Next.c)
  set(prev_empty ${CMAKE_CURRENT_BINARY_DIR}/${judy}_PrevEmpty.c)
  set(next_empty ${CMAKE_CURRENT_BINARY_DIR}/${judy}_NextEmpty.c)
  
  configure_file(JudyCommon/JudyPrevNext.c ${prev} COPYONLY)
  configure_file(JudyCommon/JudyPrevNext.c ${next} COPYONLY)
  configure_file(JudyCommon/JudyPrevNextEmpty.c ${prev_empty} COPYONLY)
  configure_file(JudyCommon/JudyPrevNextEmpty.c ${next_empty} COPYONLY)
  
  set_source_files_properties(${prev} PROPERTIES COMPILE_DEFINITIONS "JUDYPREV")
  set_source_files_properties(${prev_empty} PROPERTIES COMPILE_DEFINITIONS "JUDYPREV")
  set_source_files_properties(${next} PROPERTIES COMPILE_DEFINITIONS "JUDYNEXT")
  set_source_files_properties(${next_empty} PROPERTIES COMPILE_DEFINITIONS "JUDYNEXT")
  
  set(${judy}_sources_extra ${prev} ${next} ${prev_empty} ${next_empty} PARENT_SCOPE)
  
endfunction()

#TODO: judyget 2x
set(judy_sources JudyCommon/JudyByCount.c   
                 JudyCommon/JudyCascade.c
                 JudyCommon/JudyCount.c
                 JudyCommon/JudyCreateBranch.c
                 JudyCommon/JudyDecascade.c   
                 JudyCommon/JudyDel.c              
                 JudyCommon/JudyFirst.c            
                 JudyCommon/JudyFreeArray.c        
                 JudyCommon/JudyGet.c                   # TODO: j__udy1Test
                 JudyCommon/JudyInsArray.c    
                 JudyCommon/JudyIns.c                
                 JudyCommon/JudyInsertBranch.c         
                 JudyCommon/JudyMallocIF.c            
                 JudyCommon/JudyMemActive.c            
                 JudyCommon/JudyMemUsed.c            
                 JudyCommon/JudyMalloc.c
                 
                 JudySL/JudySL.c
                 JudyHS/JudyHS.c
)

judy_prevnext(Judy1)
judy_prevnext(JudyL)


add_library(DiabloJudy1 SHARED ${judy_sources} ${Judy1_sources_extra} Judy1Tables.c)
target_compile_definitions(DiabloJudy1
  PRIVATE JUDY1
)
target_include_directories(DiabloJudy1
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Judy1
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/JudyCommon
)

add_library(DiabloJudyL SHARED ${judy_sources} ${JudyL_sources_extra} JudyLTables.c)
target_compile_definitions(DiabloJudyL
  PRIVATE JUDYL
)
target_include_directories(DiabloJudyL
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/JudyL
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/JudyCommon
)
