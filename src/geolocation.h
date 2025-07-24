#ifndef GEOLOCATION_H
#define GEOLOCATION_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>

class GeoLocation : public QObject
{
    Q_OBJECT

public:
    explicit GeoLocation(QObject *parent = nullptr);
    ~GeoLocation();

    void requestLocation();
    void setFallbackLocation(double lat, double lon, const QString &city = QString());

signals:
    void locationReceived(double latitude, double longitude);
    void locationReceived(const QString &cityName);
    void errorOccurred(const QString &error);

private slots:
    void onNetworkReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    void processLocationResponse(const QByteArray &data);
    void useFallbackLocation();

    QNetworkAccessManager *mNetworkManager;
    QNetworkReply *mCurrentReply;

    // Fallback location
    double mFallbackLat;
    double mFallbackLon;
    QString mFallbackCity;
    bool mHasFallback;

    static const QString IP_GEOLOCATION_URL;
};

#endif // GEOLOCATION_H
