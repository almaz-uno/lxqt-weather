#ifndef GEOCODER_H
#define GEOCODER_H

#include <QByteArray>
#include <QNetworkRequest>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// The text of the "Location" setting turned into a place: coordinates written
// as text, or an address searched at a Nominatim-compatible geocoder
// (specs/002-location-by-address)

// The geocoder used unless the key geocoder_url says otherwise
extern const char *const DEFAULT_GEOCODER_URL;

struct Place
{
    double latitude = 0.0;
    double longitude = 0.0;
    QString name;
};

// Coordinates written as text — latitude and longitude in decimal degrees,
// separated by a comma or spaces: "55.6928, 37.3475". False for anything
// else, and for values out of range
bool parseCoordinates(const QString &text, Place *place);

// The name under which coordinates are shown: "55.6928, 37.3475"
QString coordinatesName(double latitude, double longitude);

// The search request for text at the geocoder at baseUrl: one result, format
// jsonv2, the User-Agent naming the widget and its version
QNetworkRequest nominatimRequest(const QString &baseUrl, const QString &text);

// The first result of a Nominatim search response: its coordinates and a short
// name — "name", or the first part of "display_name" when that is empty.
// False with *error set when there is no result or the response is malformed
bool parseNominatim(const QByteArray &json, Place *place, QString *error);

// One search at a time; a new lookup abandons the previous one
class Geocoder : public QObject
{
    Q_OBJECT

public:
    explicit Geocoder(QObject *parent = nullptr);

    void lookup(const QString &text, const QString &baseUrl);

signals:
    void found(const Place &place);
    void failed(const QString &reason);

private:
    void onFinished();

    QNetworkAccessManager *mNetwork;
    QNetworkReply *mReply;
};

#endif // GEOCODER_H
