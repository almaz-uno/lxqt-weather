#ifndef WEATHERSETTINGS_H
#define WEATHERSETTINGS_H

#include <QString>
#include <QVariant>

// Simple settings interface for the widget: the plugin adapts the panel's
// PluginSettings to it, weather-test and the tests implement it in memory.
// A header of its own, so that code using the settings needs no widget
// (specs/002-location-by-address)
class IWeatherSettings
{
public:
    virtual ~IWeatherSettings() = default;
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const = 0;
    virtual void setValue(const QString &key, const QVariant &value) = 0;
    virtual bool contains(const QString &key) const = 0;
    virtual void remove(const QString &key) = 0;
};

#endif // WEATHERSETTINGS_H
