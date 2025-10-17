#ifndef WMSIGNALRESCALE_H
#define WMSIGNALRESCALE_H


#include <QWidget>
#include <qtcontrols_global.h>

class QTCON_EXPORT wmSignalRescale : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString softChannelA READ softChannelA WRITE setSoftChannelA DESIGNABLE true)
    Q_PROPERTY(QString softChannelB READ softChannelB WRITE setSoftChannelB DESIGNABLE true)
public:
    explicit wmSignalRescale(QWidget *parent = nullptr);

    void setSoftChannelA(const QString &softChannel);
    QString softChannelA() const;

    void setSoftChannelB(const QString &softChannelB);
    QString softChannelB() const;

private:
    QString thisSoftChannelA;
    QString thisSoftChannelB;

    static bool isDesignerMode();

protected:
    void paintEvent(QPaintEvent *ev) override;

signals:
    void emitSignal(QSize size);
    void emitSignal(int width, int height);
    void emitSignalX(int width);
    void emitSignalY(int height);

};

#endif // WMSIGNALRESCALE_H
