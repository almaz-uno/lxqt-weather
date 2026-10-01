#include "locationsetting.h"

LocationSetting::LocationSetting(IWeatherSettings *settings, QObject *parent)
    : QObject(parent)
    , mSettings(settings)
    , mGeocoder(new Geocoder(this))
{
    connect(mGeocoder, &Geocoder::found, this, [this](const Place &place) {
        store(mSearchedText, place);
        emit applied();
    });
    connect(mGeocoder, &Geocoder::failed, this, &LocationSetting::failed);
}

bool LocationSetting::isSet(const IWeatherSettings *settings)
{
    return settings->contains(LOCATION_LATITUDE_KEY) && settings->contains(LOCATION_LONGITUDE_KEY);
}

QString LocationSetting::describe(const IWeatherSettings *settings)
{
    if (!isSet(settings)) {
        return QStringLiteral("Found by your IP address");
    }

    const QString coordinates = coordinatesName(settings->value(LOCATION_LATITUDE_KEY).toDouble(),
                                                settings->value(LOCATION_LONGITUDE_KEY).toDouble());
    const QString name = settings->value(LOCATION_NAME_KEY).toString();
    if (name.isEmpty() || name == coordinates) {
        return coordinates;
    }
    return QStringLiteral("%1 (%2)").arg(name, coordinates);
}

void LocationSetting::apply(const QString &text)
{
    const QString trimmed = text.trimmed();

    if (trimmed.isEmpty()) {
        mSettings->remove(LOCATION_KEY);
        mSettings->remove(LOCATION_LATITUDE_KEY);
        mSettings->remove(LOCATION_LONGITUDE_KEY);
        mSettings->remove(LOCATION_NAME_KEY);
        emit applied();
        return;
    }

    // The result of a search is kept: the same text is not searched again
    if (trimmed == mSettings->value(LOCATION_KEY).toString() && isSet(mSettings)) {
        emit applied();
        return;
    }

    Place place;
    if (parseCoordinates(trimmed, &place)) {
        store(trimmed, place);
        emit applied();
        return;
    }

    mSearchedText = trimmed;
    mGeocoder->lookup(trimmed, mSettings->value(GEOCODER_URL_KEY, DEFAULT_GEOCODER_URL).toString());
}

void LocationSetting::store(const QString &text, const Place &place)
{
    mSettings->setValue(LOCATION_KEY, text);
    mSettings->setValue(LOCATION_LATITUDE_KEY, place.latitude);
    mSettings->setValue(LOCATION_LONGITUDE_KEY, place.longitude);
    mSettings->setValue(LOCATION_NAME_KEY, place.name);
}
