#include "geolocation.h"
#include <QJsonParseError>

const QString GeoLocation::IP_GEOLOCATION_URL = "http://ip-api.com/json/";

GeoLocation::GeoLocation(QObject *parent)
    : QObject(parent)
    , mNetworkManager(nullptr)
    , mCurrentReply(nullptr)
    , mFallbackLat(0.0)
    , mFallbackLon(0.0)
    , mHasFallback(false)
{
    mNetworkManager = new QNetworkAccessManager(this);
}

GeoLocation::~GeoLocation()
{
    if (mCurrentReply && mCurrentReply->isRunning()) {
        mCurrentReply->abort();
    }
}

void GeoLocation::requestLocation()
{
    if (mCurrentReply && mCurrentReply->isRunning()) {
        mCurrentReply->abort();
    }

    QNetworkRequest request(IP_GEOLOCATION_URL);
    request.setHeader(QNetworkRequest::UserAgentHeader, "LXQt-Weather-Plugin/1.0");

    // Set timeout
    request.setTransferTimeout(10000); // 10 seconds

    mCurrentReply = mNetworkManager->get(request);

    connect(mCurrentReply, &QNetworkReply::finished,
            this, &GeoLocation::onNetworkReplyFinished);
    connect(mCurrentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &GeoLocation::onNetworkError);
}

void GeoLocation::setFallbackLocation(double lat, double lon, const QString &city)
{
    mFallbackLat = lat;
    mFallbackLon = lon;
    mFallbackCity = city;
    mHasFallback = true;
}

void GeoLocation::onNetworkReplyFinished()
{
    if (!mCurrentReply) {
        return;
    }

    if (mCurrentReply->error() == QNetworkReply::NoError) {
        QByteArray data = mCurrentReply->readAll();
        processLocationResponse(data);
    } else {
        QString errorString = mCurrentReply->errorString();
        emit errorOccurred(tr("Network error: %1").arg(errorString));
        useFallbackLocation();
    }

    mCurrentReply->deleteLater();
    mCurrentReply = nullptr;
}

void GeoLocation::onNetworkError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)

    if (mCurrentReply) {
        QString errorString = mCurrentReply->errorString();
        emit errorOccurred(tr("Network error: %1").arg(errorString));
    }

    useFallbackLocation();
}

void GeoLocation::processLocationResponse(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred(tr("JSON parse error: %1").arg(parseError.errorString()));
        useFallbackLocation();
        return;
    }

    QJsonObject obj = doc.object();

    // Check API response status
    QString status = obj["status"].toString();
    if (status != "success") {
        QString message = obj["message"].toString();
        emit errorOccurred(tr("Geolocation API error: %1").arg(message));
        useFallbackLocation();
        return;
    }

    // Extract coordinates
    if (!obj.contains("lat") || !obj.contains("lon")) {
        emit errorOccurred(tr("Invalid geolocation data received"));
        useFallbackLocation();
        return;
    }

    double lat = obj["lat"].toDouble();
    double lon = obj["lon"].toDouble();

    // Validate coordinates
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        emit errorOccurred(tr("Invalid coordinates received"));
        useFallbackLocation();
        return;
    }

    // Optional: extract city name
    QString city = obj["city"].toString();
    if (!city.isEmpty()) {
        emit locationReceived(city);
    }

    emit locationReceived(lat, lon);
}

void GeoLocation::useFallbackLocation()
{
    if (mHasFallback) {
        if (!mFallbackCity.isEmpty()) {
            emit locationReceived(mFallbackCity);
        }
        emit locationReceived(mFallbackLat, mFallbackLon);
    } else {
        emit errorOccurred(tr("No fallback location available"));
    }
}
