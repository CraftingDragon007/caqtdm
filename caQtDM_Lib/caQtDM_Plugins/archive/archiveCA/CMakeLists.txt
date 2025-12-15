if(NOT CAQTDM_HAS_CA_ARCHIVE_LIBS)
    message(FATAL_ERROR "CA archive libraries are missing. Set CAQTDM_CA_ARCHIVE_LIB_DIR or disable CAQTDM_ENABLE_ARCHIVE_CA_PLUGIN.")
endif()

set(_archive_ca_libs
    ${CAQTDM_EPICS_CORE_LIBRARIES}
    ${CAQTDM_CA_STORAGE_LIBRARY}
    ${CAQTDM_CA_TOOLS_LIBRARY}
    ${CAQTDM_CA_XERCES_LIBRARY})
if(CAQTDM_IS_64BIT)
    list(APPEND _archive_ca_libs curl)
endif()

set(_archive_ca_includes
    ${CAQTDM_CA_ARCHIVE_LIB_DIR}/Tools
    ${CAQTDM_CA_ARCHIVE_LIB_DIR}/Storage
    ${CAQTDM_CA_ARCHIVE_LIB_DIR}/xerces-c-3.1.4/src)

caqtdm_add_plugin(archiveCA_plugin
    SOURCES
        archiveCA_plugin.cpp
        ../archiverCommon.cpp
    QT_COMPONENTS
        Network
    EXTRA_INCLUDES
        ${_archive_ca_includes}
    EXTRA_LIBS
        ${_archive_ca_libs})
