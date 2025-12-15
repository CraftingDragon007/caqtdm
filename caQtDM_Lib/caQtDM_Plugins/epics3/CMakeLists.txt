caqtdm_add_plugin(epics3_plugin
    SOURCES
        epics3_plugin.cpp
        epicsSubs.c
    EXTRA_LIBS
        ${CAQTDM_EPICS_CORE_LIBRARIES}
        ${CAQTDM_EPICS_LIB_COM}
        ${CAQTDM_EPICS_LIB_DB})
