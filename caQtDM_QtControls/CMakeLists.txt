# qtcontrols: the widget library (mirrors caQtDM_QtControls.pro).
# On Linux the prc parser is linked statically into libqtcontrols.so so the
# library stays self-contained (no DT_NEEDED on libprcParser.so).

set(_src "${CMAKE_CURRENT_SOURCE_DIR}/src")

# --------------------------------------------------------------------------------------------------
# Sources / headers
# --------------------------------------------------------------------------------------------------
set(QTCONTROLS_SOURCES
    ${_src}/caframe.cpp
    ${_src}/cainclude.cpp
    ${_src}/caimage.cpp
    ${_src}/cagraphics.cpp
    ${_src}/capolyline.cpp
    ${_src}/searchfile.cpp
    ${_src}/elabel.cpp
    ${_src}/esimplelabel.cpp
    ${_src}/fontscalingwidget.cpp
    ${_src}/calabel.cpp
    ${_src}/imagepushbutton.cpp
    ${_src}/epushbutton.cpp
    ${_src}/enumeric.cpp
    ${_src}/leftclick_with_modifiers_eater.cpp
    ${_src}/number_delegate.cpp
    ${_src}/dec_int_from_format.cpp
    ${_src}/eapplybutton.cpp
    ${_src}/camenu.cpp
    ${_src}/camessagebutton.cpp
    ${_src}/cashellcommand.cpp
    ${_src}/carowcolmenu.cpp
    ${_src}/carelateddisplay.cpp
    ${_src}/caapplynumeric.cpp
    ${_src}/canumeric.cpp
    ${_src}/eapplynumeric.cpp
    ${_src}/cachoice.cpp
    ${_src}/catextentry.cpp
    ${_src}/calineedit.cpp
    ${_src}/caled.cpp
    ${_src}/eled.cpp
    ${_src}/cabitnames.cpp
    ${_src}/eflag.cpp
    ${_src}/catable.cpp
    ${_src}/cabyte.cpp
    ${_src}/rectangle.cpp
    ${_src}/cagauge.cpp
    ${_src}/egauge.cpp
    ${_src}/eng_notation.cpp
    ${_src}/cathermo.cpp
    ${_src}/caslider.cpp
    ${_src}/cacartesianplot.cpp
    ${_src}/castripplot.cpp
    ${_src}/cacamera.cpp
    ${_src}/imagewidget.cpp
    ${_src}/cacalc.cpp
    ${_src}/gensoftpv.cpp
    ${_src}/parsepepfile.cpp
    ${_src}/uiconverter.cpp
    ${_src}/catogglebutton.cpp
    ${_src}/cascriptbutton.cpp
    ${_src}/cadoubletabwidget.cpp
    ${_src}/stripplotthread.cpp
    ${_src}/cawaterfallplot.cpp
    ${_src}/snumeric.cpp
    ${_src}/caspinbox.cpp
    ${_src}/qwtplotcurvenan.cpp
    ${_src}/cawavetable.cpp
    ${_src}/cawavetablemodel.cpp
    ${_src}/specialFunctions.cpp
    ${_src}/caclock.cpp
    ${_src}/cameter.cpp
    ${_src}/colormaps.cpp
    ${_src}/cascan2d.cpp
    ${_src}/mdaReader.cpp
    ${_src}/mda_loader.c
    ${_src}/messageQueue.cpp
    ${_src}/cabytecontroller.cpp
    ${_src}/calabelvertical.cpp
    ${_src}/camultilinestring.cpp
    ${_src}/camimedisplay.cpp
    ${_src}/calinedraw.cpp
    ${_src}/wmsignalpropagator.cpp
    ${_src}/replacemacro.cpp
    ${_src}/textedit.cpp
    ${_src}/wmsignalrescale.cpp
    ${_src}/networkaccess.cpp
    ${_src}/fileFunctions.cpp
    ${_src}/hmiapplicationeventfilter.cpp
    ${_src}/cahmiconfig.cpp
    ${_src}/cahmiconfigtransferitem.cpp)

set(QTCONTROLS_HEADERS
    ${_src}/networkaccess.h
    ${_src}/fileFunctions.h
    ${_src}/calinedraw.h
    ${_src}/plotHelperClasses.h
    ${_src}/wmsignalpropagator.h
    ${_src}/replacemacro.h
    ${_src}/networkmodel.h
    ${_src}/textedit.h
    ${_src}/wmsignalrescale.h
    ${_src}/hmiapplicationeventfilter.h
    ${_src}/cahmiconfig.h
    ${_src}/cahmiconfigtransferitem.h
    ${_src}/caframe.h
    ${_src}/cainclude.h
    ${_src}/caimage.h
    ${_src}/cagraphics.h
    ${_src}/capolyline.h
    ${_src}/searchfile.h
    ${_src}/elabel.h
    ${_src}/esimplelabel.h
    ${_src}/fontscalingwidget.h
    ${_src}/calabel.h
    ${_src}/imagepushbutton.h
    ${_src}/epushbutton.h
    ${_src}/enumeric.h
    ${_src}/number_delegate.h
    ${_src}/leftclick_with_modifiers_eater.h
    ${_src}/dec_int_from_format.h
    ${_src}/camenu.h
    ${_src}/camessagebutton.h
    ${_src}/cashellcommand.h
    ${_src}/carowcolmenu.h
    ${_src}/carelateddisplay.h
    ${_src}/canumeric.h
    ${_src}/caapplynumeric.h
    ${_src}/eapplynumeric.h
    ${_src}/cachoice.h
    ${_src}/catextentry.h
    ${_src}/calineedit.h
    ${_src}/caled.h
    ${_src}/eled.h
    ${_src}/cabitnames.h
    ${_src}/eflag.h
    ${_src}/catable.h
    ${_src}/cabyte.h
    ${_src}/rectangle.h
    ${_src}/cagauge.h
    ${_src}/egauge.h
    ${_src}/eng_notation.h
    ${_src}/cathermo.h
    ${_src}/caslider.h
    ${_src}/castripplot.h
    ${_src}/cacartesianplot.h
    ${_src}/cacamera.h
    ${_src}/imagewidget.h
    ${_src}/cacalc.h
    ${_src}/gensoftpv.h
    ${_src}/qtcontrols_global.h
    ${_src}/alarmdefs.h
    ${_src}/parsepepfile.h
    ${_src}/uiconverter.h
    ${_src}/catogglebutton.h
    ${_src}/cascriptbutton.h
    ${_src}/cadoubletabwidget.h
    ${_src}/stripplotthread.h
    ${_src}/cawaterfallplot.h
    ${_src}/snumeric.h
    ${_src}/caspinbox.h
    ${_src}/qwtplotcurvenan.h
    ${_src}/cawavetable.h
    ${_src}/cawavetablemodel.h
    ${_src}/capolylinedialog.h
    ${_src}/specialFunctions.h
    ${_src}/caclock.h
    ${_src}/cameter.h
    ${_src}/colormaps.h
    ${_src}/cascan2d.h
    ${_src}/mdaReader.h
    ${_src}/mda-load.h
    ${_src}/messageQueue.h
    ${_src}/cabytecontroller.h
    ${_src}/qtdefinitions.h
    ${_src}/pathdefinitions.h
    ${_src}/camultilinestring.h
    ${_src}/formattype.h
    ${_src}/calabelvertical.h
    ${_src}/camimedisplay.h
    ${_src}/caWidgetInterface.h
    ${_src}/addevent.h
    ${_src}/animationcode.h
    ${_src}/hideobjectcode.h)

if(Qwt_VERSION_MINOR EQUAL 0)
    list(APPEND QTCONTROLS_SOURCES ${_src}/qwt_thermo_marker.cpp)
    list(APPEND QTCONTROLS_HEADERS ${_src}/qwt_thermo_marker.h)
else()
    list(APPEND QTCONTROLS_SOURCES ${_src}/qwt_thermo_marker_61.cpp)
    list(APPEND QTCONTROLS_HEADERS ${_src}/qwt_thermo_marker_61.h)
endif()

if(CAQTDM_ADL_EDL)
    list(APPEND QTCONTROLS_SOURCES ${_src}/parseotherfile.cpp)
    list(APPEND QTCONTROLS_HEADERS ${_src}/parseotherfile.h)
endif()

list(APPEND QTCONTROLS_SOURCES ${_src}/xdr_hack.c)
list(APPEND QTCONTROLS_HEADERS ${_src}/xdr_hack.h)

if(NOT CAQTDM_MOBILE)
    list(APPEND QTCONTROLS_SOURCES
        ${_src}/pvtaskmenu.cpp
        ${_src}/pvdialog.cpp
        ${_src}/gensoftpvtaskmenu.cpp
        ${_src}/gensoftpvdialog.cpp
        ${_src}/cadoubletabwidgetextensionfactory.cpp
        ${_src}/cadoubletabwidgetextension.cpp
        ${_src}/capolylinetaskmenu.cpp
        ${_src}/capolylinedialog.cpp)
    list(APPEND QTCONTROLS_HEADERS
        ${_src}/pvtaskmenu.h
        ${_src}/pvdialog.h
        ${_src}/gensoftpvtaskmenu.h
        ${_src}/gensoftpvdialog.h
        ${_src}/cadoubletabwidgetextension.h
        ${_src}/cadoubletabwidgetextensionfactory.h
        ${_src}/capolylinetaskmenu.h)
endif()

# --------------------------------------------------------------------------------------------------
# Target
# --------------------------------------------------------------------------------------------------
if(CAQTDM_MOBILE)
    add_library(qtcontrols STATIC ${QTCONTROLS_SOURCES} ${QTCONTROLS_HEADERS})
else()
    add_library(qtcontrols SHARED ${QTCONTROLS_SOURCES} ${QTCONTROLS_HEADERS})
endif()

add_library(caqtdm::qtcontrols ALIAS qtcontrols)

target_include_directories(qtcontrols
    PUBLIC
        "${_src}"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/prcParserSrc")

target_link_libraries(qtcontrols
    PUBLIC
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::OpenGL
        Qt6::Network
        Qt6::Xml
        Qt6::UiTools
        Qwt::Qwt
    PRIVATE
        prcParserStatic)

if(CAQTDM_ADL_EDL)
    target_link_libraries(qtcontrols PRIVATE adlParserStatic)
    if(UNIX)
        target_link_libraries(qtcontrols PRIVATE edlParserStatic)
    endif()
endif()

if(NOT CAQTDM_MOBILE)
    target_link_libraries(qtcontrols PRIVATE Qt6::Designer)
endif()

if(APPLE)
    find_package(ZLIB REQUIRED)
    target_link_libraries(qtcontrols PRIVATE ZLIB::ZLIB)
endif()

target_compile_definitions(qtcontrols PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x050000)

if(MSVC)
    target_compile_definitions(qtcontrols PRIVATE QTCON_MAKEDLL _CRT_SECURE_NO_WARNINGS)
    caqtdm_add_version_rc(qtcontrols
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qtcontrols.rc" "Widgets for Display Manager" "qtcontrols.dll")
endif()

caqtdm_set_output_dir(qtcontrols)
caqtdm_apply_rpath(qtcontrols)

install(TARGETS qtcontrols
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# --------------------------------------------------------------------------------------------------
# Designer plugins (desktop only; mobile builds them statically into the app)
# --------------------------------------------------------------------------------------------------
if(NOT CAQTDM_MOBILE)
    add_subdirectory(plugins)
endif()
