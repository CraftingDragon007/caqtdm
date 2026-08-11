/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2026
 *
 *  Author:
 *    Erik Schwarz
 *  Contact details:
 *    erik.schwarz@psi.ch
 */

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

    // setRequest/setUrl/setAttribute are protected in QNetworkReply
    void configureAsHttpOk(const QNetworkRequest &request)
    {
        setRequest(request);
        setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
    }

    // NetworkAccess only checks error() and the two http attributes
    void configureAsHttpError(const QNetworkRequest &request, int statusCode, const QString &reasonPhrase)
    {
        setRequest(request);
        setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reasonPhrase);
        setError(QNetworkReply::ContentNotFoundError, reasonPhrase);
    }

    // marks the reply complete; the manager turns this into its own finished(reply)
    void triggerFinished()
    {
        setFinished(true);
        emit readyRead();
        emit finished();
    }

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
