#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include <QObject>
#include <QtOpcUa/QOpcUaClient>
#include <QtOpcUa/QOpcUaProvider>
#include <QtOpcUa/QOpcUaNode>
#include <QtOpcUa/QOpcUaAddReferenceItem>
#include <QtOpcUa/QOpcUaExpandedNodeId>
#include <QtOpcUa/QOpcUaClient>
#include <QtOpcUa/QOpcUaEndpointDescription>
#include <QtOpcUa/QOpcUaBrowseRequest>
#include <QtOpcUa/QOpcUaReferenceDescription>
#include <QtOpcUa/QOpcUaQualifiedName>
#include <QtOpcUa/QOpcUaLocalizedText>
#include <QUrl>
#include <QTimer>

namespace opc{

    class OpcUaCore : public QObject
    {
        Q_OBJECT

    public:
        explicit OpcUaCore(QObject *parent = nullptr);
        ~OpcUaCore();

        bool connectOpc(const QString &url);
        void disconnectOpc();
        void fetchDataFromAnyNode();
        void fetchDataFromSingleNode(const QString &nodeId);
        void fetchDataFromMultipleNodes(const QStringList &nodeIds);
        void browseRoot();

    signals:
        void connected();
        void disconnected();
        void errorOccured(const QString &message);
        void valueRead(const QString nodeId, const QVariant &value);
        void valuesRead(const QVector<QVariant> &values);

    private:
        QOpcUaProvider m_provider;
        QOpcUaClient *m_client;
        bool m_endpointsHooked = false;
        bool isClientConnected();
        void browseObjectForVariables(const QString &objectNodeId);
        void QOpcUaBrowseResult(QOpcUaNode *, void (*)(QVector<QOpcUaReferenceDescription>, QOpcUa::UaStatusCode), OpcUaCore *, QDebug);
};
}

#endif // OPCUA_CLIENT_H
