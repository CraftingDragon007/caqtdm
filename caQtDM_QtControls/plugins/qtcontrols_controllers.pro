include(./plugins.pri)

SOURCES	+= qtcontrols_controllers_plugin.cpp
HEADERS	+= qtcontrols_controllers_plugin.h  designerPluginTexts.h  plugin_xml_helper.h
RESOURCES += qtcontrolsplugin.qrc
TARGET = qtcontrols_controllers_plugin

PLUGIN_BUILD_DIR = $$OUT_PWD/$$TARGET

MOC_DIR     = $$PLUGIN_BUILD_DIR/moc
OBJECTS_DIR  = $$PLUGIN_BUILD_DIR/obj
RCC_DIR      = $$PLUGIN_BUILD_DIR/rcc
UI_DIR       = $$PLUGIN_BUILD_DIR/ui

android {
   INCLUDEPATH += $(ANDROIDFUNCTIONSINCLUDE)
}
