#include "weatherapi.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QDebug>

// Open-Meteo API - бесплатный и без ключей!
const QString WeatherAPI::API_BASE_URL = "https://api.open-meteo.com/v1/forecast";

WeatherAPI::WeatherAPI(QObject *parent)
    : QObject(parent)
    , mNetworkManager(new QNetworkAccessManager(this))
{
    connect(mNetworkManager, &QNetworkAccessManager::finished,
            this, &WeatherAPI::onNetworkReplyFinished);
}

void WeatherAPI::requestWeatherByCoordinates(double latitude, double longitude)
{
    QUrl url(API_BASE_URL);
    QUrlQuery query;

    // Open-Meteo параметры
    query.addQueryItem("latitude", QString::number(latitude, 'f', 6));
    query.addQueryItem("longitude", QString::number(longitude, 'f', 6));
    query.addQueryItem("current_weather", "true");
    query.addQueryItem("hourly", "temperature_2m,relative_humidity_2m,weather_code");
    query.addQueryItem("daily", "weather_code,temperature_2m_max,temperature_2m_min");
    query.addQueryItem("timezone", "auto");

    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "LXQt-Weather-Widget/1.0");

    QNetworkReply *reply = mNetworkManager->get(request);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &WeatherAPI::onNetworkError);

    qDebug() << "Requesting weather data from Open-Meteo:" << url.toString();
}

void WeatherAPI::requestWeatherByCity(const QString &cityName)
{
    // Open-Meteo работает только с координатами
    // Для городов нужно использовать геокодирование отдельно
    Q_UNUSED(cityName)
    emit errorOccurred("Open-Meteo requires coordinates. Please use location service.");
}

void WeatherAPI::onNetworkReplyFinished(QNetworkReply *reply)
{
    if (!reply) {
        qWarning() << "onNetworkReplyFinished: reply is null";
        return;
    }

    qDebug() << "Network reply finished. URL:" << reply->url();
    qDebug() << "HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error occurred:" << reply->error() << reply->errorString();
        emit errorOccurred(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    qDebug() << "Received data size:" << data.size() << "bytes";

    if (data.isEmpty()) {
        qWarning() << "Received empty data from API";
        emit errorOccurred("Empty response from weather API");
        return;
    }

    qDebug() << "Processing weather data...";
    processWeatherData(data);
}

void WeatherAPI::onNetworkError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        emit errorOccurred(QString("Network error: %1").arg(reply->errorString()));
        reply->deleteLater();
    }
}

void WeatherAPI::processWeatherData(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred(QString("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject root = doc.object();

    // Проверяем наличие данных о текущей погоде
    if (!root.contains("current_weather")) {
        emit errorOccurred("Invalid response format from Open-Meteo API");
        return;
    }

    QJsonObject currentWeather = root["current_weather"].toObject();

    // Преобразуем формат Open-Meteo в наш внутренний формат
    QJsonObject weatherData;
    QJsonObject main;
    QJsonArray weatherArray;
    QJsonObject weather;
    QJsonObject wind;

    // Основные данные о погоде
    main["temp"] = currentWeather["temperature"].toDouble();
    main["humidity"] = 0; // Open-Meteo не возвращает влажность в current_weather
    main["pressure"] = 0; // Также нет давления в current_weather

    // Ветер
    wind["speed"] = currentWeather["windspeed"].toDouble();
    wind["deg"] = currentWeather["winddirection"].toDouble();

    // Описание погоды на основе WMO кода
    int weatherCode = currentWeather["weathercode"].toInt();
    weather["id"] = weatherCode;
    weather["main"] = getWeatherDescription(weatherCode);
    weather["description"] = getWeatherDescription(weatherCode);
    weather["icon"] = getWeatherIconFromCode(weatherCode);

    weatherArray.append(weather);

    // Собираем финальный объект
    weatherData["main"] = main;
    weatherData["weather"] = weatherArray;
    weatherData["wind"] = wind;
    weatherData["dt"] = QDateTime::currentSecsSinceEpoch();
    weatherData["name"] = "Current Location";

    // Добавляем координаты
    QJsonObject coord;
    coord["lat"] = root["latitude"].toDouble();
    coord["lon"] = root["longitude"].toDouble();
    weatherData["coord"] = coord;

    qDebug() << "Weather data processed successfully";
    qDebug() << "Emitting weatherDataReceived signal with temperature:" << main["temp"].toDouble();
    emit weatherDataReceived(weatherData);
}

QString WeatherAPI::getWeatherIconFromCode(int weatherCode) const
{
    // Маппинг WMO weather codes в иконки
    switch (weatherCode) {
        case 0: return "01d"; // Clear sky
        case 1: return "02d"; // Mainly clear
        case 2: return "03d"; // Partly cloudy
        case 3: return "04d"; // Overcast
        case 45:
        case 48: return "50d"; // Fog
        case 51:
        case 53:
        case 55: return "09d"; // Drizzle
        case 56:
        case 57: return "13d"; // Freezing drizzle
        case 61:
        case 63:
        case 65: return "10d"; // Rain
        case 66:
        case 67: return "13d"; // Freezing rain
        case 71:
        case 73:
        case 75: return "13d"; // Snow fall
        case 77: return "13d"; // Snow grains
        case 80:
        case 81:
        case 82: return "09d"; // Rain showers
        case 85:
        case 86: return "13d"; // Snow showers
        case 95: return "11d"; // Thunderstorm
        case 96:
        case 99: return "11d"; // Thunderstorm with hail
        default: return "02d"; // Default partly cloudy
    }
}

QString WeatherAPI::getWeatherDescription(int weatherCode) const
{
    // Описания на основе WMO кодов
    switch (weatherCode) {
        case 0: return "Clear sky";
        case 1: return "Mainly clear";
        case 2: return "Partly cloudy";
        case 3: return "Overcast";
        case 45: return "Fog";
        case 48: return "Depositing rime fog";
        case 51: return "Light drizzle";
        case 53: return "Moderate drizzle";
        case 55: return "Dense drizzle";
        case 56: return "Light freezing drizzle";
        case 57: return "Dense freezing drizzle";
        case 61: return "Slight rain";
        case 63: return "Moderate rain";
        case 65: return "Heavy rain";
        case 66: return "Light freezing rain";
        case 67: return "Heavy freezing rain";
        case 71: return "Slight snow fall";
        case 73: return "Moderate snow fall";
        case 75: return "Heavy snow fall";
        case 77: return "Snow grains";
        case 80: return "Slight rain showers";
        case 81: return "Moderate rain showers";
        case 82: return "Violent rain showers";
        case 85: return "Slight snow showers";
        case 86: return "Heavy snow showers";
        case 95: return "Thunderstorm";
        case 96: return "Thunderstorm with slight hail";
        case 99: return "Thunderstorm with heavy hail";
        default: return "Unknown";
    }
}
