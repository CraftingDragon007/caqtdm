include(./plugins.pri)

SOURCES	+= qtcontrols_monitors_plugin.cpp
HEADERS	+= qtcontrols_monitors_plugin.h designerPluginTexts.h  plugin_xml_helper.h
!MOBILE{
RESOURCES += qtcontrolsplugin.qrc
}
TARGET = qtcontrols_monitors_plugin

PLUGIN_BUILD_DIR = $$OUT_PWD/$$TARGET

MOC_DIR     = $$PLUGIN_BUILD_DIR/moc
OBJECTS_DIR  = $$PLUGIN_BUILD_DIR/obj
RCC_DIR      = $$PLUGIN_BUILD_DIR/rcc
UI_DIR       = $$PLUGIN_BUILD_DIR/ui
