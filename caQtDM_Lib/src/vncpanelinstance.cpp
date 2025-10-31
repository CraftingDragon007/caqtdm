#include "vncpanelinstance.h"

QDataStream& operator<<(QDataStream &out, const VNCPanelInstance& vncPanelInstance) {
    out << vncPanelInstance.m_panel;
    out << vncPanelInstance.m_port_suffix;
    out << vncPanelInstance.m_timestamp;
    return out;
}

QDataStream& operator>>(QDataStream &in, VNCPanelInstance& vncPanelInstance) {
    in >> vncPanelInstance.m_panel;
    in >> vncPanelInstance.m_port_suffix;
    in >> vncPanelInstance.m_timestamp;
    return in;
}
