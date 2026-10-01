#ifndef LXQTWEATHERWIDGET_H
#define LXQTWEATHERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkConfigurationManager>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QVariant>

#include "weathersettings.h"

// Forward declarations
class WeatherAPI;
class GeoLocation;

class LXQtWeatherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LXQtWeatherWidget(QWidget *parent = nullptr);
    ~LXQtWeatherWidget();

    void updateSettings(IWeatherSettings *settings);
    QSize sizeHint() const override;

public slots:
    void refreshWeather();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onWeatherDataReceived(const QJsonObject &data);
    void onLocationReceived(double latitude, double longitude);
    void onLocationReceived(const QString &cityName);
    void onUpdateTimer();
    void onNetworkConfigurationChanged();

private:
    void setupUI();
    void setupServices();
    void updateDisplay(const QString &description = QString(), const QString &iconCode = QString());
    void updateTooltip(const QJsonObject &data);
    void openYandexWeatherMap();

    QString formatTemperature(double temperature) const;
    QString getWeatherDescription(int weatherCode) const;
    QPixmap loadWeatherIcon(const QString &iconCode) const;
    double getScaleFactor() const { return logicalDpiX() / 96.0; }

private:
    // Services
    WeatherAPI *mWeatherAPI;
    GeoLocation *mGeoLocation;
    QTimer *mUpdateTimer;
    QNetworkConfigurationManager *mNetworkManager;

    // UI components
    QLabel *mIconLabel;
    QLabel *mTemperatureLabel;
    QLabel *mDescriptionLabel;

    // Settings
    int mUpdateInterval;        // In minutes
    QString mTemperatureUnit;   // "celsius" or "fahrenheit"
    bool mShowDescription;

    // Current data
    double mCurrentTemperature;
    QString mCurrentCity;
    double mCurrentLatitude;
    double mCurrentLongitude;
    bool mHasValidData;

    // The location set in the settings, if any (specs/002-location-by-address)
    bool mHasSetLocation;
    bool mSettingsApplied;      // updateSettings() has run before
};

#endif // LXQTWEATHERWIDGET_H
