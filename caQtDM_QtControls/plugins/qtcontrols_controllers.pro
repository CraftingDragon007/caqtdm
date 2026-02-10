include(./plugins.pri)

SOURCES	+= qtcontrols_controllers_plugin.cpp
HEADERS	+= qtcontrols_controllers_plugin.h  designerPluginTexts.h  plugin_xml_helper.h
RESOURCES += qtcontrolsplugin.qrc
TARGET = qtcontrols_controllers_plugin

android {
   INCLUDEPATH += $(ANDROIDFUNCTIONSINCLUDE)
}
