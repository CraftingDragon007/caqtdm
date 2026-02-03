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
    # Disable two-phase name lookup to help with template compilation
    target_compile_options(epics4_plugin PRIVATE /Zc:twoPhase-)
    
    # Remove /permissive- flag that causes template errors in EPICS pvData
    # Get current compile options
    get_target_property(_compile_opts epics4_plugin COMPILE_OPTIONS)
    if(_compile_opts)
        # Remove /permissive- from the list
        list(FILTER _compile_opts EXCLUDE REGEX "^[/-]permissive-$")
        set_target_properties(epics4_plugin PROPERTIES COMPILE_OPTIONS "${_compile_opts}")
    endif()
    
    # Also check and remove from interface compile options if present
    get_target_property(_iface_opts epics4_plugin INTERFACE_COMPILE_OPTIONS)
    if(_iface_opts)
        list(FILTER _iface_opts EXCLUDE REGEX "^[/-]permissive-$")
        set_target_properties(epics4_plugin PROPERTIES INTERFACE_COMPILE_OPTIONS "${_iface_opts}")
    endif()
endif()
