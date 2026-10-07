#include <QApplication>
#include <QWidget>
#include <QUiLoader>
#include <QPluginLoader>
#include <QDir>
#include <QDebug>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QUiLoader loader;
    const QStringList available = loader.availableWidgets();
    bool ok = true;
    foreach (const QString &name, QStringList() << "caLineEdit" << "caNumeric"
             << "caLabel" << "caShellCommand") {
        QWidget *widget = loader.createWidget(name);
        if (!available.contains(name) || !widget ||
            QString(widget->metaObject()->className()) != name) {
            qCritical() << "Widget unavailable:" << name << loader.errorString();
            ok = false;
        } else {
            qWarning() << "Widget instantiated:" << name;
        }
        delete widget;
    }
    foreach (const QString &name, QStringList() << "epics3" << "epics4") {
        QPluginLoader plugin(QDir(app.applicationDirPath()).filePath(
            "controlsystems/" + name + "_plugin.dll"));
        if (!plugin.instance()) {
            qCritical() << "Plugin failed:" << name << plugin.errorString();
            ok = false;
        } else {
            qWarning() << "Control-system plugin loaded:" << name;
        }
    }
    return ok ? 0 : 1;
}
