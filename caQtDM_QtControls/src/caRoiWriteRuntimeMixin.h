#ifndef CAROIWRITERUNTIMEMIXIN_H
#define CAROIWRITERUNTIMEMIXIN_H

#include <QStringList>
#include <QPointF>

class caRoiWriteRuntimeInterface
{
public:
    struct Request {
        bool active;
        QStringList channels;
        int count;
        double values[4];

        Request() : active(false), count(4), values{0.0, 0.0, 0.0, 0.0} {}
    };

    virtual ~caRoiWriteRuntimeInterface() {}
    virtual Request roiWriteRequest() = 0;
};

template <typename Widget>
class caRoiWriteRuntimeMixin : public caRoiWriteRuntimeInterface
{
public:
    Request roiWriteRequest() override
    {
        Widget &widget = static_cast<Widget &>(*this);
        const int type = static_cast<int>(widget.getROIwriteType());
        if (type == 0) // none
            return Request();

        Request request;
        request.channels = widget.getROIChannelsWrite().split(";");

        QPointF p1, p2;
        widget.getROI(p1, p2);
        const double dx = p2.x() - p1.x();
        const double dy = p2.y() - p1.y();

        switch (type) {
        case 1: // xy_only
            request.count = 2;
            request.values[0] = p1.x();
            request.values[1] = p1.y();
            break;
        case 2: // xy1_xy2
            request.values[0] = p1.x();
            request.values[1] = p1.y();
            request.values[2] = p2.x();
            request.values[3] = p2.y();
            break;
        case 3: // xyUpleft_xyLowright
            if (dx < 0 && dy > 0) {
                request.values[0] = p2.x();
                request.values[1] = p1.y();
                request.values[2] = p1.x();
                request.values[3] = p2.y();
            } else if (dx < 0 && dy < 0) {
                request.values[0] = p2.x();
                request.values[1] = p2.y();
                request.values[2] = p1.x();
                request.values[3] = p1.y();
            } else if (dx > 0 && dy < 0) {
                request.values[0] = p1.x();
                request.values[1] = p2.y();
                request.values[2] = p2.x();
                request.values[3] = p1.y();
            } else if (dx > 0 && dy > 0) {
                request.values[0] = p1.x();
                request.values[1] = p1.y();
                request.values[2] = p2.x();
                request.values[3] = p2.y();
            }
            break;
        case 4: { // xycenter_width_height
            double x = p1.x();
            double y = p1.y();
            double w = p2.x() - p1.x();
            double h = p2.y() - p1.y();
            if (w < 0) { x += w; w = -w; }
            if (h < 0) { y += h; h = -h; }
            request.values[0] = x + w / 2.0;
            request.values[1] = y + h / 2.0;
            request.values[2] = w;
            request.values[3] = h;
            break;
        }
        default:
            return Request();
        }

        request.active = true;
        return request;
    }
};

#endif // CAROIWRITERUNTIMEMIXIN_H
