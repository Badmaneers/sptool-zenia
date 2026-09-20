#include "NetworkOps.h"
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QDateTime>
#include <QTimer>
#include "Logger/Log.h"
#include "../JSONParser/json.h"

#define AT_SERVER_URL "http://mcdat06.gcn.mediatek.inc:8083/api/flash/"
#define NET_TIMEOUT 6 * 1000
#define API_KEY "Api-Key xSp7E8Ly.pWR6U65ubUAhyUMYmPMQKlc4Wjs6rVBB"

NetworkOps::NetworkOps()
{
    m_net_manager = QSharedPointer<QNetworkAccessManager>(new QNetworkAccessManager());
    //connect(m_net_manager.get(), SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));
}

bool NetworkOps::post(const QByteArray &postData) const
{
    QTimer timer;
    timer.setInterval(NET_TIMEOUT); // 6 second
    timer.setSingleShot(true);

    QString baseUrl = QString::fromUtf8(AT_SERVER_URL);
    //QString::fromUtf8("http://mcdat06.gcn.mediatek.inc:9090/api/echo");
    //baseUrl = "http://172.26.113.29:8888/api/echo";
    QUrl url(baseUrl);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", QString::fromUtf8(API_KEY).toLocal8Bit());
    request.setUrl(url);

    QDateTime utc_datetime = QDateTime::currentDateTimeUtc();
    utc_datetime.setTimeSpec(Qt::UTC);
    LOG("test: device tracking post data: %s", QString(postData).toStdString().c_str());
    LOG("test: datetimebegin of UTC: %s", utc_datetime.toString("yyyy-MM-ddTHH:mm:ssZ").toStdString().c_str());
    QNetworkReply *reply = m_net_manager->post(request, postData);
    reply->setParent(m_net_manager.data()); //deleted by parent

    QEventLoop eventLoop;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    timer.start();
    eventLoop.exec();
    LOG("test: datetimeend of UTC: %s", utc_datetime.toString("yyyy-MM-ddTHH:mm:ssZ").toStdString().c_str());

    if (timer.isActive()) { // reponse finished
        timer.stop();
        return postFinished(reply);
    } else { // timeout
        disconnect(reply, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
        reply->abort();
        //reply->deleteLater();
        LOGI("network timeout out");
        return false;
    }
}

#include <QFile>
bool NetworkOps::get() const
{
    QTimer timer;
    timer.setInterval(NET_TIMEOUT); // 6 second
    timer.setSingleShot(true);

    QString baseUrl = QString::fromUtf8(AT_SERVER_URL);
    //QString::fromUtf8("http://mcdat06.gcn.mediatek.inc:9090/api/echo");
    //baseUrl = "http://172.26.113.29:8888/api/echo";
    QUrl url(baseUrl);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", QString::fromUtf8(API_KEY).toLocal8Bit());
    request.setUrl(url);

    QDateTime utc_datetime = QDateTime::currentDateTimeUtc();
    utc_datetime.setTimeSpec(Qt::UTC);
    LOG("test: datetimebegin of UTC: %s", utc_datetime.toString("yyyy-MM-ddTHH:mm:ssZ").toStdString().c_str());
    QNetworkReply *reply = m_net_manager->get(request);
    reply->setParent(m_net_manager.data()); //deleted by parent

    QEventLoop eventLoop;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    timer.start();
    eventLoop.exec();
    LOG("test: datetimeend of UTC: %s", utc_datetime.toString("yyyy-MM-ddTHH:mm:ssZ").toStdString().c_str());

    if (timer.isActive()) { // reponse finished
        timer.stop();
        return getFinished(reply);
    } else { // timeout
        disconnect(reply, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
        reply->abort();
        //reply->deleteLater();
        LOGI("network timeout out");
        return false;
    }
}

bool NetworkOps::postFinished(QNetworkReply *reply) const
{
    if(reply->error() != QNetworkReply::NoError) {
        LOGE("POST operation failed, error string: %s", reply->errorString().toStdString().c_str());
        return false;
    }

    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status_code != 201) {
        LOG("POST operation failed, status code: %d", status_code);
        return false;
    }
    LOG("POST operation success!");

    //LOG("test: url: %s", reply->url().toString().toStdString().c_str());

    const QByteArray reply_data=reply->readAll();
    LOG("test: POST operation receive reponse data: %s", QString(reply_data).toStdString().c_str());

    /*
    if (reply_data.isEmpty()) {
        LOGE("received reponse data is EMPTY!!!");
        return false;
    }

    bool success = false;
    QtJson::JsonObject json_obj = QtJson::parse(reply_data, success).toMap();
    if (!success) {
        LOGE("received reponse data is NOT a json data!!!");
        return false;
    }

    if (json_obj.contains("datetime")) {
        LOG("reponse datetime: %s", json_obj["datetime"].toString().toStdString().c_str());
    }
    if (json_obj.contains("device_id")) {
        LOG("reponse device_id: %s", json_obj["device_id"].toString().toStdString().c_str());
    }
    */

    //reply->deleteLater();
    return true;
}

bool NetworkOps::getFinished(QNetworkReply *reply) const
{
    if(reply->error() != QNetworkReply::NoError) {
        LOGE("GET operation failed, error string: %s", reply->errorString().toStdString().c_str());
        return false;
    }

    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status_code != 200) {
        LOG("GET operation failed, status code: %d", status_code);
        return false;
    }
    LOG("GET operation success!");

    LOG("test: url: %s", reply->url().toString().toStdString().c_str());

    const QByteArray reply_data = reply->readAll();
    LOG("test: GET operation receive reponse data: %s", QString(reply_data).toStdString().c_str());


    reply->deleteLater();
    return true;
}
