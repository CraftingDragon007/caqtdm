if(NOT CAQTDM_ENABLE_EPICS4_PLUGIN)
    return()
endif()

# MSVC: Remove /permissive- from CMAKE_CXX_FLAGS for this subdirectory only
# This must be done BEFORE creating any targets
if(MSVC)
    # Save original flags
    set(_epics4_orig_cxx_flags "${CMAKE_CXX_FLAGS}")
    
    # Remove /permissive- from this subdirectory's CMAKE_CXX_FLAGS
    string(REGEX REPLACE "[/-]permissive-" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    
    message(STATUS "EPICS4: Modified CMAKE_CXX_FLAGS to remove /permissive-")
endif()

set(_epics4_required_libs pvAccess pvAccessCA pvData pvaClient nt)
set(_epics4_link_libs ${CAQTDM_EPICS_CORE_LIBRARIES})
foreach(_lib IN LISTS _epics4_required_libs)
    find_library(_epics4_${_lib}_path
        NAMES ${_lib} lib${_lib}
        HINTS ${CAQTDM_EPICS_LIBRARY_DIR})
    if(NOT _epics4_${_lib}_path)
        message(FATAL_ERROR "Could not locate EPICS4 library '${_lib}' inside ${CAQTDM_EPICS_LIBRARY_DIR}.")
    endif()
    list(APPEND _epics4_link_libs ${_epics4_${_lib}_path})
endforeach()

caqtdm_add_plugin(epics4_plugin
    SOURCES
        epics4_plugin.cpp
        epics4_callbackThread.cpp
        epics4Requester.cpp
    EXTRA_LIBS
        ${_epics4_link_libs})

target_compile_definitions(epics4_plugin PRIVATE QT_NO_WARNING_OUTPUT QT_NO_DEBUG_OUTPUT)

# MSVC-specific compiler flags for EPICS4
if(MSVC)
    # EPICS pvData headers are not compatible with strict MSVC conformance mode
    # /permissive- was already removed from CMAKE_CXX_FLAGS at the top of this file
    
    # Disable two-phase name lookup to help with template compilation
    target_compile_options(epics4_plugin PRIVATE /Zc:twoPhase-)
endif()
