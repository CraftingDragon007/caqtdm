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
 */

#ifndef FAKENETWORKACCESSMANAGER_H
#define FAKENETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "fakenetworkreply.h"

// Serves a fixed payload instead of doing http, so the real download path
// of NetworkAccess can be exercised without a server.
class FakeNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    explicit FakeNetworkAccessManager(const QByteArray &payload, QObject *parent = Q_NULLPTR)
        : QNetworkAccessManager(parent)
        , m_payload(payload)
        , m_requestCount(0)
        , m_httpStatus(200)
        , m_neverFinish(false)
    {}

    int requestCount() const { return m_requestCount; }
    QUrl lastRequestedUrl() const { return m_lastUrl; }

    // subsequent requests answer with an http error instead of the payload
    void failWithHttpStatus(int statusCode, const QString &reasonPhrase)
    {
        m_httpStatus = statusCode;
        m_reasonPhrase = reasonPhrase;
    }

    // subsequent requests never complete, so the caller runs into its timeout
    void neverFinish() { m_neverFinish = true; }

protected:
    QNetworkReply *createRequest(Operation op,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData = Q_NULLPTR) override
    {
        Q_UNUSED(op)
        Q_UNUSED(outgoingData)

        m_requestCount++;
        m_lastUrl = request.url();

        FakeNetworkReply *reply = new FakeNetworkReply(m_payload, this);
        if(m_httpStatus == 200) reply->configureAsHttpOk(request);
        else reply->configureAsHttpError(request, m_httpStatus, m_reasonPhrase);

        // finish asynchronously, the caller is still inside get()
        if(!m_neverFinish) {
            QTimer::singleShot(0, reply, [reply]() { reply->triggerFinished(); });
        }
        return reply;
    }

private:
    QByteArray m_payload;
    int m_requestCount;
    QUrl m_lastUrl;
    int m_httpStatus;
    QString m_reasonPhrase;
    bool m_neverFinish;
};

#endif // FAKENETWORKACCESSMANAGER_H
