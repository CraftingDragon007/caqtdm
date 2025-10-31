#ifndef VNCPANELINSTANCE_H
#define VNCPANELINSTANCE_H

#include "qobject.h"
#include "qobjectdefs.h"
#include "qtmetamacros.h"
#include "caQtDM_Lib_global.h"

struct CAQTDM_LIBSHARED_EXPORT VNCPanelInstance {
    Q_GADGET
    Q_PROPERTY(QString panel MEMBER m_panel)
    Q_PROPERTY(quint16 portSuffix MEMBER m_port_suffix)
    Q_PROPERTY(qint64 timestamp MEMBER m_timestamp)

public:
    QString m_panel;
    quint16 m_port_suffix;
    qint64 m_timestamp;

};

Q_DECLARE_METATYPE(VNCPanelInstance)

CAQTDM_LIBSHARED_EXPORT QDataStream& operator<<(QDataStream& out, const VNCPanelInstance& vncPanelInstance);
CAQTDM_LIBSHARED_EXPORT QDataStream& operator>>(QDataStream& in, VNCPanelInstance& vncPanelInstance);

#endif // VNCPANELINSTANCE_H
