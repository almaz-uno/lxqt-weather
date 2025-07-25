#ifndef WEATHERAPI_H
#define WEATHERAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

class WeatherAPI : public QObject
{
    Q_OBJECT

public:
    explicit WeatherAPI(QObject *parent = nullptr);

    // Убираем API ключ - Open-Meteo не требует регистрации
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

    // Open-Meteo API endpoint
    static const QString API_BASE_URL;

    void processWeatherData(const QByteArray &data);
    QString getWeatherIconFromCode(int weatherCode) const;
    QString getWeatherDescription(int weatherCode) const;
};

#endif // WEATHERAPI_H
