#ifndef LOCATIONSETTING_H
#define LOCATIONSETTING_H

#include <QObject>
#include <QString>

#include "geocoder.h"
#include "weathersettings.h"

// The keys of the "Location" setting in the plugin's settings
// (specs/002-location-by-address): the text as entered, and the place it
// stands for — what the widget uses
constexpr const char LOCATION_KEY[] = "location";
constexpr const char LOCATION_LATITUDE_KEY[] = "location_latitude";
constexpr const char LOCATION_LONGITUDE_KEY[] = "location_longitude";
constexpr const char LOCATION_NAME_KEY[] = "location_name";
constexpr const char GEOCODER_URL_KEY[] = "geocoder_url";

// The "Location" field of the settings dialogs applied to the settings
class LocationSetting : public QObject
{
    Q_OBJECT

public:
    explicit LocationSetting(IWeatherSettings *settings, QObject *parent = nullptr);

    // Whether a place is stored, and what the stored setting stands for:
    // "Технопарк (55.6928, 37.3475)", or that the location is found by IP
    static bool isSet(const IWeatherSettings *settings);
    static QString describe(const IWeatherSettings *settings);

    // The text of the field applied: empty removes the setting, text
    // unchanged since it was saved keeps it, coordinates are stored as they
    // are, other text is searched. applied() or failed() follows, at once or
    // after the search — connect before calling
    void apply(const QString &text);

signals:
    void applied();
    void failed(const QString &reason);

private:
    void store(const QString &text, const Place &place);

    IWeatherSettings *mSettings;
    Geocoder *mGeocoder;
    QString mSearchedText;
};

#endif // LOCATIONSETTING_H
