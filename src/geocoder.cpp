#include "geocoder.h"
#include "version.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

const char *const DEFAULT_GEOCODER_URL = "https://nominatim.openstreetmap.org";

bool parseCoordinates(const QString &text, Place *place)
{
    static const QRegularExpression coordinates(QStringLiteral(
        "^\\s*([+-]?\\d+(?:\\.\\d+)?)\\s*(?:,\\s*|\\s+)([+-]?\\d+(?:\\.\\d+)?)\\s*$"));

    const QRegularExpressionMatch match = coordinates.match(text);
    if (!match.hasMatch()) {
        return false;
    }
    const double latitude = match.captured(1).toDouble();
    const double longitude = match.captured(2).toDouble();
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        return false;
    }

    place->latitude = latitude;
    place->longitude = longitude;
    place->name = coordinatesName(latitude, longitude);
    return true;
}

QString coordinatesName(double latitude, double longitude)
{
    return QStringLiteral("%1, %2").arg(latitude, 0, 'f', 4).arg(longitude, 0, 'f', 4);
}

QNetworkRequest nominatimRequest(const QString &baseUrl, const QString &text)
{
    QString base = baseUrl;
    while (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }

    QUrl url(base + QStringLiteral("/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), text);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("jsonv2"));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
    url.setQuery(query);

    // The Nominatim usage policy asks for a User-Agent naming the application
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "lxqt-weather/" LXQT_WEATHER_VERSION);
    request.setTransferTimeout(10000); // 10 seconds
    return request;
}

bool parseNominatim(const QByteArray &json, Place *place, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        *error = QStringLiteral("malformed geocoder response");
        return false;
    }

    const QJsonArray results = document.array();
    if (results.isEmpty()) {
        *error = QStringLiteral("not found");
        return false;
    }

    // Nominatim gives the coordinates as strings
    const QJsonObject first = results.first().toObject();
    bool latitudeOk = false;
    bool longitudeOk = false;
    const double latitude = first.value(QStringLiteral("lat")).toString().toDouble(&latitudeOk);
    const double longitude = first.value(QStringLiteral("lon")).toString().toDouble(&longitudeOk);
    if (!latitudeOk || !longitudeOk) {
        *error = QStringLiteral("malformed geocoder response");
        return false;
    }

    QString name = first.value(QStringLiteral("name")).toString().trimmed();
    if (name.isEmpty()) {
        name = first.value(QStringLiteral("display_name")).toString().section(QLatin1Char(','), 0, 0).trimmed();
    }
    if (name.isEmpty()) {
        name = coordinatesName(latitude, longitude);
    }

    place->latitude = latitude;
    place->longitude = longitude;
    place->name = name;
    return true;
}

Geocoder::Geocoder(QObject *parent)
    : QObject(parent)
    , mNetwork(new QNetworkAccessManager(this))
    , mReply(nullptr)
{
}

void Geocoder::lookup(const QString &text, const QString &baseUrl)
{
    if (mReply) {
        mReply->disconnect(this);
        mReply->abort();
        mReply->deleteLater();
    }

    mReply = mNetwork->get(nominatimRequest(baseUrl, text));
    connect(mReply, &QNetworkReply::finished, this, &Geocoder::onFinished);
}

void Geocoder::onFinished()
{
    QNetworkReply *reply = mReply;
    mReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit failed(reply->errorString());
        return;
    }

    Place place;
    QString error;
    if (parseNominatim(reply->readAll(), &place, &error)) {
        emit found(place);
    } else {
        emit failed(error);
    }
}
