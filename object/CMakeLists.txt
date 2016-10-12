# TODO: PECOFF
option(ObjectELF "Base requirement: ELF Objects" ON)
option(ObjectTICOFF "TICOFF Objects" ON)
option(ObjectPECOFF "PECOFF (Windows) objects" OFF)
option(MapARMADS "ARMADS Map files" ON)
option(MapBinUtils "Base requirement: BinUtils Map files" ON)
option(MapTILinker "TILinker Map files" ON)
option(ArchiveAr "Base requirement: Ar Archives" ON)

# TODO: MapARMADS can't depend on ArchitectureARM since that is defined in flowgraph, unless we
# move all options together, which might not be a bad thing anyway

mark_as_advanced(ObjectELF ObjectTICOFF)
#mark_as_advanced(ObjectPECOFF)
mark_as_advanced(MapARMADS MapBinUtils MapTILinker)
mark_as_advanced(ArchiveAr)

# TODO: the CACHE INTERNAL is fugly, FIXME
set_defined_from_config(HAVE_ARM_ADS_SUPPORT MapARMADS)
set_defined_from_config(HAVE_BINUTILS_SUPPORT MapBinUtils)
set_defined_from_config(HAVE_TILINKER_SUPPORT MapTILinker)
set_defined_from_config(HAVE_AR_SUPPORT ArchiveAr)
set_defined_from_config(HAVE_ELF_SUPPORT ObjectELF)
set_defined_from_config(HAVE_TICOFF_SUPPORT ObjectTICOFF)
set_defined_from_config(HAVE_PECOFF_SUPPORT ObjectPECOFF)

add_subdirectory(data)
add_subdirectory(diabloobject)
add_subdirectory(map_backends)
add_subdirectory(object_backends)
add_subdirectory(archive_backends)
add_subdirectory(debug_backends)

set (DIABLOOBJECT_VERSION_MAJOR 0)
set (DIABLOOBJECT_VERSION_MINOR 5)

use_global_config(DIABLOOBJECT HAVE_SYS_WAIT_H)

configure_diablo_config(object)
