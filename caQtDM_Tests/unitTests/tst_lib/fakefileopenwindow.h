#ifndef FAKEFILEOPENWINDOW_H
#define FAKEFILEOPENWINDOW_H

#include <QMainWindow>

class FakeFileOpenWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FakeFileOpenWindow(QWidget *parent = Q_NULLPTR)
        : QMainWindow(parent)
    {}

public slots:
    void Callback_IosExit() { qInfo() << "Callback_IosExit"; }
    void Callback_ReloadWindow(QWidget *) { qInfo() << "Callback_ReloadWindow"; }
    void Callback_ReloadAllWindows() { qInfo() << "Callback_ReloadAllWindows"; }
    void Callback_OpenNewFile(const QString &, const QString &, const QString &, const QString &)
    {
        qInfo() << "Callback_OpenNewFile";
    }
};

#endif // FAKEFILEOPENWINDOW_H
