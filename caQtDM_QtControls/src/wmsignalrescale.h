#ifndef WMSIGNALRESCALE_H
#define WMSIGNALRESCALE_H


#include <QWidget>
#include <qtcontrols_global.h>

class QTCON_EXPORT wmSignalRescale : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString softChannelA READ softChannelA WRITE setSoftChannelA DESIGNABLE true)
    Q_PROPERTY(QString softChannelB READ softChannelB WRITE setSoftChannelB DESIGNABLE true)
    Q_PROPERTY(QPoint rectSignalPosition READ rectSignalPosition WRITE setRectSignalPosition DESIGNABLE true)
public:
    explicit wmSignalRescale(QWidget *parent = nullptr);

    void setSoftChannelA(const QString &softChannel);
    QString softChannelA() const;

    void setSoftChannelB(const QString &softChannelB);
    QString softChannelB() const;

    void setRectSignalPosition(const QPoint &point);
    QPoint rectSignalPosition() const;

private:
    QString thisSoftChannelA;
    QString thisSoftChannelB;
    QPoint thisRectSignalPosition;

    static bool isDesignerMode();

protected:
    void paintEvent(QPaintEvent *ev) override;
    bool eventFilter(QObject *target, QEvent *event) override;

signals:
    void emitSignal(QRect rect);
    void emitSignal(QSize size);
    void emitSignal(int width, int height);
    void emitSignalWidth(int width);
    void emitSignalHeight(int height);
    void internalResizeEvent(QObject* target, QWidget* wmSignalRescaleWidget, QResizeEvent *event, QString channelA, QString channelB);

};

#endif // WMSIGNALRESCALE_H
