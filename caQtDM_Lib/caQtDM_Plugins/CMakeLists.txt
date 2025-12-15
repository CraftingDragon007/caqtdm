include(CaQtDMPluginHelpers)

add_subdirectory(demo)
add_subdirectory(environment)
add_subdirectory(epics3)
if(CAQTDM_ENABLE_EPICS4_PLUGIN)
    add_subdirectory(epics4)
endif()

if(CAQTDM_ENABLE_ARCHIVE_PLUGINS)
    add_subdirectory(archive)
endif()

if(CAQTDM_ENABLE_BSREAD_PLUGIN AND NOT CAQTDM_ENABLE_MOBILE)
    add_subdirectory(bsread)
endif()

if(CAQTDM_ENABLE_MODBUS_PLUGIN)
    if(QT_VERSION_MAJOR GREATER 5 OR (QT_VERSION_MAJOR EQUAL 5 AND CAQTDM_QT_VERSION_MINOR_NUM GREATER 10))
        add_subdirectory(modbus)
    else()
        message(WARNING "Modbus plugin requires Qt >= 5.11; skipping")
    endif()
endif()

if(CAQTDM_ENABLE_GPS_PLUGIN)
    add_subdirectory(gps)
endif()
