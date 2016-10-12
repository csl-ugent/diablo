# Make three target libraries, one for each generated pair of option files
opt_gen_ext_base_make_library(Common ${CMAKE_CURRENT_SOURCE_DIR} common global_options global_list Global diablo_options "-d")
opt_gen_ext_base_make_library(ARM ${CMAKE_CURRENT_SOURCE_DIR} diablo_arm arm_options arm_options_list ArmOptions diablo_arm_options "-d")
opt_gen_ext_base_make_library(I386 ${CMAKE_CURRENT_SOURCE_DIR} diablo_i386 i386_options i386_options_list I386Options diablo_i386_options "-d")
