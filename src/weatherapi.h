#ifndef WEATHERAPI_H
#define WEATHERAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>

// The request of the current weather at a place: the block "current" of
// Open-Meteo with the temperature, the relative humidity, the surface
// pressure, the weather code and the wind in m/s (specs/005-humidity-pressure)
QUrl openMeteoUrl(double latitude, double longitude);

// An Open-Meteo response turned into the object the widget reads: main.temp
// (°C), main.humidity (%), main.pressure (hPa, at the surface), wind.speed
// (m/s), wind.deg, weather[0] with the code, the description and the icon
// code, coord. False with *error set when the block "current" is missing or
// the response is malformed
bool parseOpenMeteo(const QByteArray &json, QJsonObject *weather, QString *error);

class WeatherAPI : public QObject
{
    Q_OBJECT

public:
    explicit WeatherAPI(QObject *parent = nullptr);

    // Remove API key - Open-Meteo doesn't require registration
    void requestWeatherByCoordinates(double latitude, double longitude);
    void requestWeatherByCity(const QString &cityName);
    void setCityName(const QString &cityName);

signals:
    void weatherDataReceived(const QJsonObject &data);
    void errorOccurred(const QString &error);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply);
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *mNetworkManager;
    QString mCityName;
    QTimer *mRetryTimer;
    int mRetryCount;
    double mLastLatitude;
    double mLastLongitude;

    void processWeatherData(const QByteArray &data);
    void retryRequest();
};

#endif // WEATHERAPI_H
