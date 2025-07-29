#include "weatherapi.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QDebug>

// Open-Meteo API - free and no keys required!
const QString WeatherAPI::API_BASE_URL = "http://api.open-meteo.com/v1/forecast";

WeatherAPI::WeatherAPI(QObject *parent)
    : QObject(parent)
    , mNetworkManager(new QNetworkAccessManager(this))
    , mCityName("")
    , mRetryTimer(new QTimer(this))
    , mRetryCount(0)
    , mLastLatitude(0.0)
    , mLastLongitude(0.0)
{
    connect(mNetworkManager, &QNetworkAccessManager::finished,
            this, &WeatherAPI::onNetworkReplyFinished);

    // Setup retry timer
    mRetryTimer->setSingleShot(true);
    connect(mRetryTimer, &QTimer::timeout, this, &WeatherAPI::retryRequest);
}

void WeatherAPI::requestWeatherByCoordinates(double latitude, double longitude)
{
    // Save coordinates for retry attempts
    mLastLatitude = latitude;
    mLastLongitude = longitude;
    mRetryCount = 0; // Reset retry count for new request

    // Stop any pending retry timer
    if (mRetryTimer->isActive()) {
        mRetryTimer->stop();
    }

    QUrl url(API_BASE_URL);
    QUrlQuery query;

    // Open-Meteo parameters
    query.addQueryItem("latitude", QString::number(latitude, 'f', 6));
    query.addQueryItem("longitude", QString::number(longitude, 'f', 6));
    query.addQueryItem("current_weather", "true");
    query.addQueryItem("hourly", "temperature_2m,relative_humidity_2m,weather_code");
    query.addQueryItem("daily", "weather_code,temperature_2m_max,temperature_2m_min");
    query.addQueryItem("timezone", "auto");

    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "LXQt-Weather-Widget/1.0");
    
    // Set timeout to 10 seconds to avoid long waits
    request.setTransferTimeout(10000);
    
    // Set raw headers for better compatibility - don't ask for compression
    request.setRawHeader("Accept", "application/json");
    // Don't set Accept-Encoding to avoid compression issues

    QNetworkReply *reply = mNetworkManager->get(request);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &WeatherAPI::onNetworkError);

    qDebug() << "Requesting weather data from Open-Meteo:" << url.toString();
}

void WeatherAPI::requestWeatherByCity(const QString &cityName)
{
    // Open-Meteo works only with coordinates
    // For cities, geocoding needs to be used separately
    Q_UNUSED(cityName)
    emit errorOccurred("Open-Meteo requires coordinates. Please use location service.");
}

void WeatherAPI::setCityName(const QString &cityName)
{
    mCityName = cityName;
    qDebug() << "City name set to:" << cityName;
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

        // Check if this is a retryable error and we haven't exceeded retry limit
        bool shouldRetry = false;
        QNetworkReply::NetworkError error = reply->error();
        if ((error == QNetworkReply::TimeoutError ||
             error == QNetworkReply::HostNotFoundError ||
             error == QNetworkReply::NetworkSessionFailedError ||
             error == QNetworkReply::TemporaryNetworkFailureError) &&
             mRetryCount < 2) {
            shouldRetry = true;
        }

        if (shouldRetry) {
            mRetryCount++;
            int retryDelay = mRetryCount * 5000; // 5s, 10s delays

            qDebug() << "Scheduling retry attempt" << mRetryCount << "in" << retryDelay << "ms";
            mRetryTimer->start(retryDelay);
            return;
        }

        // No retry or exceeded retry limit
        emit errorOccurred(reply->errorString());
        return;
    }

    // Success - reset retry count
    mRetryCount = 0;

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
    if (!reply) return;

    QString errorString = reply->errorString();
    qWarning() << "Network error occurred:" << error << errorString;

    // Check if this is a timeout or network connectivity error
    bool shouldRetry = false;
    if (error == QNetworkReply::TimeoutError ||
        error == QNetworkReply::HostNotFoundError ||
        error == QNetworkReply::NetworkSessionFailedError ||
        error == QNetworkReply::TemporaryNetworkFailureError) {
        shouldRetry = true;
    }

    // Retry up to 2 times with exponential backoff
    if (shouldRetry && mRetryCount < 2) {
        mRetryCount++;
        int retryDelay = mRetryCount * 5000; // 5s, 10s delays

        qDebug() << "Scheduling retry attempt" << mRetryCount << "in" << retryDelay << "ms";
        mRetryTimer->start(retryDelay);

        reply->deleteLater();
        return;
    }

    // No more retries or non-retryable error
    emit errorOccurred(errorString);
    reply->deleteLater();
}

void WeatherAPI::processWeatherData(const QByteArray &data)
{
    QByteArray processedData = data;
    
    // Check if data is gzip compressed (starts with \x1f\x8b or looks like binary)
    if (data.size() > 2 && ((unsigned char)data[0] == 0x1f && (unsigned char)data[1] == 0x8b)) {
        qDebug() << "Data is gzip compressed, decompressing...";
        processedData = qUncompress(data);
        if (processedData.isEmpty()) {
            emit errorOccurred("Failed to decompress gzip data");
            return;
        }
    } else if (data.size() > 0 && (unsigned char)data[0] == 0x78) {
        // Check for zlib/deflate compression (starts with 0x78)
        qDebug() << "Data is zlib compressed, decompressing...";
        processedData = qUncompress(data);
        if (processedData.isEmpty()) {
            emit errorOccurred("Failed to decompress zlib data");
            return;
        }
    }
    
    qDebug() << "Raw data received:" << processedData.left(200) << "..."; // Show first 200 chars
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(processedData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error at offset" << parseError.offset << ":" << parseError.errorString();
        qWarning() << "Data around error:" << processedData.mid(qMax(0, parseError.offset - 20), 40);
        emit errorOccurred(QString("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject root = doc.object();

    // Check for current weather data
    if (!root.contains("current_weather")) {
        emit errorOccurred("Invalid response format from Open-Meteo API");
        return;
    }

    QJsonObject currentWeather = root["current_weather"].toObject();

    // Convert Open-Meteo format to our internal format
    QJsonObject weatherData;
    QJsonObject main;
    QJsonArray weatherArray;
    QJsonObject weather;
    QJsonObject wind;

    // Main weather data
    main["temp"] = currentWeather["temperature"].toDouble();
    main["humidity"] = 0; // Open-Meteo doesn't return humidity in current_weather
    main["pressure"] = 0; // Also no pressure in current_weather

    // Wind
    wind["speed"] = currentWeather["windspeed"].toDouble();
    wind["deg"] = currentWeather["winddirection"].toDouble();

    // Weather description based on WMO code
    int weatherCode = currentWeather["weathercode"].toInt();
    weather["id"] = weatherCode;
    weather["main"] = getWeatherDescription(weatherCode);
    weather["description"] = getWeatherDescription(weatherCode);
    weather["icon"] = getWeatherIconFromCode(weatherCode);

    weatherArray.append(weather);

    // Build final object
    weatherData["main"] = main;
    weatherData["weather"] = weatherArray;
    weatherData["wind"] = wind;
    weatherData["dt"] = QDateTime::currentSecsSinceEpoch();

    // Use real city name if available
    if (!mCityName.isEmpty()) {
        weatherData["name"] = mCityName;
    } else {
        weatherData["name"] = "Current Location";
    }

    // Add coordinates
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
    // Mapping WMO weather codes to icons
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
    // Descriptions based on WMO codes
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

void WeatherAPI::retryRequest()
{
    qDebug() << "Retrying weather request, attempt" << mRetryCount;

    if (mLastLatitude != 0.0 || mLastLongitude != 0.0) {
        requestWeatherByCoordinates(mLastLatitude, mLastLongitude);
    }
}
