# set PCH for VS project
function(ADD_PRECOMPILED_HEADER TargetName PrecompiledHeader PrecompiledSource)
  if(MSVC)
    get_filename_component(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")

    set_target_properties(${TargetName}
      PROPERTIES
        COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
        OBJECT_DEPENDS "${PrecompiledBinary}")
    set_source_files_properties(${PrecompiledSource}
      PROPERTIES
        COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
        OBJECT_OUTPUTS "${PrecompiledBinary}")
  endif(MSVC)
endfunction(ADD_PRECOMPILED_HEADER)

# ignore PCH for a specified list of files
function(IGNORE_PRECOMPILED_HEADER SourcesVar)
  if(MSVC)  
    set_source_files_properties(${${SourcesVar}} PROPERTIES COMPILE_FLAGS "/Y-")
  endif(MSVC)
endfunction(IGNORE_PRECOMPILED_HEADER)
