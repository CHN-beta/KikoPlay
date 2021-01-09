#ifndef NETWORK_H
#define NETWORK_H
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QtCore>
namespace Network
{
    const int timeout=10000;
    QByteArray httpGet(const QString &url, const QUrlQuery &query, const QStringList &header=QStringList(), int ttl=10);
    QByteArray httpPost(const QString &url, const QByteArray &data, const QStringList &header=QStringList());
    QList<QPair<QString,QByteArray> > httpGetBatch(const QStringList &urls, const QList<QUrlQuery> &querys, const QList<QStringList> &headers=QList<QStringList>());
    QJsonDocument toJson(const QString &str);
    QJsonValue getValue(QJsonObject &obj, const QString &path);
    int decompress(const QByteArray &input, QByteArray &output);
    int gzipCompress(const QByteArray &input, QByteArray &output);
    int gzipDecompress(const QByteArray &input, QByteArray &output);
    class NetworkError
    {
    public:
        NetworkError(QString info):errorInfo(info){}
        QString errorInfo;
    };
}

#endif // NETWORK_H
