if(NOT CAQTDM_LOGGING_ARCHIVE_LIBRARY)
    message(FATAL_ERROR "Logging archive library not configured. Set CAQTDM_LOGGING_ARCHIVE_LIB_DIR or disable CAQTDM_ENABLE_ARCHIVE_PRO_PLUGIN.")
endif()

caqtdm_add_plugin(archivePRO_plugin
    SOURCES
        archivePRO_plugin.cpp
        ../archiverCommon.cpp
        proRetrieval.c
    QT_COMPONENTS
        Network
    EXTRA_INCLUDES
        ${CAQTDM_LOGGING_ARCHIVE_LIB_DIR}
    EXTRA_LIBS
        ${CAQTDM_LOGGING_ARCHIVE_LIBRARY})
