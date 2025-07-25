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

// Forward declarations
class WeatherAPI;
class GeoLocation;

// Простой интерфейс настроек для виджета
class IWeatherSettings
{
public:
    virtual ~IWeatherSettings() = default;
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const = 0;
    virtual void setValue(const QString &key, const QVariant &value) = 0;
    virtual bool contains(const QString &key) const = 0;
};

class LXQtWeatherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LXQtWeatherWidget(QWidget *parent = nullptr);
    ~LXQtWeatherWidget();

    void updateSettings(IWeatherSettings *settings);

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
    int mUpdateInterval;        // В минутах
    QString mTemperatureUnit;   // "celsius" или "fahrenheit"
    bool mShowDescription;

    // Current data
    double mCurrentTemperature;
    QString mCurrentCity;
    double mCurrentLatitude;
    double mCurrentLongitude;
    bool mHasValidData;
};

#endif // LXQTWEATHERWIDGET_H
