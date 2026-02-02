if(NOT CAQTDM_ENABLE_EPICS4_PLUGIN)
    return()
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
    target_compile_options(epics4_plugin PRIVATE
        /Zc:twoPhase-
        /Zc:referenceBinding)
endif()
