#ifndef FAKENETWORKREPLY_H
#define FAKENETWORKREPLY_H

#include <QNetworkReply>

class FakeNetworkReply : public QNetworkReply
{
public:
    explicit FakeNetworkReply(const QByteArray &data, QObject *parent = Q_NULLPTR)
        : QNetworkReply(parent)
        , m_data(data)
        , m_offset(0)
    {
        setOpenMode(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    void triggerReadyRead() { emit readyRead(); }

    void abort() override {}

    qint64 bytesAvailable() const override
    {
        return (m_data.size() - m_offset) + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *buffer, qint64 maxlen) override
    {
        if (m_offset >= m_data.size()) {
            return 0;
        }

        qint64 length = qMin(maxlen, static_cast<qint64>(m_data.size() - m_offset));
        memcpy(buffer, m_data.constData() + m_offset, static_cast<size_t>(length));
        m_offset += length;
        return length;
    }

private:
    QByteArray m_data;
    qint64 m_offset;
};

#endif // FAKENETWORKREPLY_H
