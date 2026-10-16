#ifndef CAPLOT_RESIZE_RUNTIME_MIXIN_H
#define CAPLOT_RESIZE_RUNTIME_MIXIN_H

#include <QFont>
#include <QVariantList>
#include <qwt_plot.h>

class caPlotResizeRuntime
{
public:
    virtual ~caPlotResizeRuntime() = default;
    virtual void capturePlotResizePresentation(QVariantList &geometry) = 0;
    virtual void resizePlotPresentation(qreal tickFontSize, qreal titleFontSize,
                                        qreal legendFontSize, double factX, double factY,
                                        const QVariantList &geometry) = 0;
};

template <typename Widget>
class caPlotResizeRuntimeMixin : public caPlotResizeRuntime
{
public:
    void capturePlotResizePresentation(QVariantList &geometry) override
    {
        Widget *widget = static_cast<Widget *>(this);
        QwtPlot *plot = widget;
        geometry.insert(4, plot->axisFont(QwtPlot::xBottom).pointSize());
        geometry.insert(5, 9);
        geometry.insert(6, plot->axisTitle(QwtPlot::xBottom).font().pointSize());
        widget->capturePlotResizeDetails(geometry);
        geometry.insert(8, plot->axisScaleDraw(QwtPlot::xBottom)->tickLength(QwtScaleDiv::MajorTick));
        geometry.insert(9, plot->axisScaleDraw(QwtPlot::xBottom)->tickLength(QwtScaleDiv::MediumTick));
        geometry.insert(10, plot->axisScaleDraw(QwtPlot::xBottom)->tickLength(QwtScaleDiv::MinorTick));
    }

    void resizePlotPresentation(qreal tickFontSize, qreal titleFontSize,
                                qreal legendFontSize, double factX, double factY,
                                const QVariantList &geometry) override
    {
        Widget *widget = static_cast<Widget *>(this);
        QwtPlot *plot = widget;

        QFont font = plot->axisFont(QwtPlot::xBottom);
        font.setPointSizeF(tickFontSize);
        plot->setAxisFont(QwtPlot::xBottom, font);
        plot->setAxisFont(QwtPlot::yLeft, font);

        font = plot->title().font();
        QwtText title = plot->title().text();
        QwtText titleX = plot->axisTitle(QwtPlot::xBottom).text();
        QwtText titleY = plot->axisTitle(QwtPlot::yLeft).text();
        font.setPointSizeF(titleFontSize);
        title.setFont(font);
        titleX.setFont(font);
        titleY.setFont(font);
        plot->setTitle(title);
        plot->setAxisTitle(QwtPlot::xBottom, titleX);
        plot->setAxisTitle(QwtPlot::yLeft, titleY);

        font.setPointSizeF(legendFontSize);
        widget->resizePlotResizeDetails(font, factX, factY, geometry);
    }
};

#endif // CAPLOT_RESIZE_RUNTIME_MIXIN_H
