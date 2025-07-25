#include "lxqtweatherwidget.h"
#include "weatherapi.h"
#include "geolocation.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QToolTip>

LXQtWeatherWidget::LXQtWeatherWidget(QWidget *parent)
    : QWidget(parent)
    , mWeatherAPI(nullptr)
    , mGeoLocation(nullptr)
    , mUpdateTimer(nullptr)
    , mNetworkManager(nullptr)
    , mIconLabel(nullptr)
    , mTemperatureLabel(nullptr)
    , mDescriptionLabel(nullptr)
    , mUpdateInterval(30)
    , mTemperatureUnit("celsius")
    , mShowDescription(true)
    , mCurrentTemperature(0.0)
    , mCurrentCity("")
    , mHasValidData(false)
{
    setupUI();
    setupServices();

    // Начальное обновление через 2 секунды после создания
    QTimer::singleShot(2000, this, &LXQtWeatherWidget::refreshWeather);
}

LXQtWeatherWidget::~LXQtWeatherWidget()
{
    if (mUpdateTimer && mUpdateTimer->isActive()) {
        mUpdateTimer->stop();
    }
}

void LXQtWeatherWidget::updateSettings(IWeatherSettings *settings)
{
    if (!settings) return;

    mUpdateInterval = settings->value("update_interval", 30).toInt();
    mTemperatureUnit = settings->value("temperature_unit", "celsius").toString();
    mShowDescription = settings->value("show_description", true).toBool();

    // Обновляем таймер обновления
    if (mUpdateTimer) {
        mUpdateTimer->stop();
        mUpdateTimer->setInterval(mUpdateInterval * 60 * 1000); // Конвертируем в миллисекунды
        mUpdateTimer->start();
    }

    // Обновляем отображение описания
    if (mDescriptionLabel) {
        mDescriptionLabel->setVisible(mShowDescription);
    }

    // Обновляем отображение температуры если есть данные
    if (mHasValidData) {
        mTemperatureLabel->setText(formatTemperature(mCurrentTemperature));
    }

    qDebug() << "Settings updated: interval =" << mUpdateInterval
             << "min, unit =" << mTemperatureUnit
             << ", show desc =" << mShowDescription;
}

void LXQtWeatherWidget::refreshWeather()
{
    if (!mGeoLocation || !mWeatherAPI) {
        qWarning() << "Services not initialized";
        return;
    }

    qDebug() << "Refreshing weather data...";

    // Запрашиваем геолокацию, которая затем запросит погодные данные
    mGeoLocation->requestLocation();
}

void LXQtWeatherWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (!mHasValidData) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(QPen(QColor(100, 100, 100), 1));
        painter.setFont(QFont("Arial", 10));
    }
}

void LXQtWeatherWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        refreshWeather();
    }
    QWidget::mousePressEvent(event);
}

void LXQtWeatherWidget::onWeatherDataReceived(const QJsonObject &data)
{
    qDebug() << "Weather data received:" << data;

    if (!data.contains("main") || !data.contains("weather")) {
        qWarning() << "Invalid weather data format";
        return;
    }

    QJsonObject main = data["main"].toObject();
    QJsonArray weather = data["weather"].toArray();

    if (weather.isEmpty()) {
        qWarning() << "No weather information available";
        return;
    }

    QJsonObject weatherInfo = weather[0].toObject();

    // Извлекаем данные
    mCurrentTemperature = main["temp"].toDouble();
    QString description = weatherInfo["description"].toString();
    QString iconCode = weatherInfo["icon"].toString();

    // ВАЖНО: Сначала устанавливаем флаг валидности данных
    mHasValidData = true;

    // Затем обновляем UI
    updateDisplay(description, iconCode);

    // Обновляем tooltip с подробной информацией
    updateTooltip(data);

    qDebug() << "Weather updated: temp =" << mCurrentTemperature
             << "°C, desc =" << description;
}

void LXQtWeatherWidget::onLocationReceived(double latitude, double longitude)
{
    qDebug() << "Location received:" << latitude << "," << longitude;

    if (mWeatherAPI) {
        // Используем новый метод для Open-Meteo
        mWeatherAPI->requestWeatherByCoordinates(latitude, longitude);
    }
}

void LXQtWeatherWidget::onLocationReceived(const QString &cityName)
{
    qDebug() << "City name received:" << cityName;
    mCurrentCity = cityName;

    // Передаем название города в WeatherAPI
    if (mWeatherAPI) {
        mWeatherAPI->setCityName(cityName);
    }

    // Обновляем тултип если есть данные о погоде
    if (mHasValidData && mWeatherAPI) {
        // Можно обновить тултип с новым названием города
        updateTooltip(QJsonObject()); // Передаем пустой объект, так как данные уже есть
    }
}

void LXQtWeatherWidget::onUpdateTimer()
{
    refreshWeather();
}

void LXQtWeatherWidget::onNetworkConfigurationChanged()
{
    qDebug() << "Network configuration changed - updating location...";

    if (!mGeoLocation) {
        qWarning() << "Geolocation service not initialized";
        return;
    }

    // При изменении сети сразу обновляем геолокацию
    // Это поможет при включении/выключении VPN
    mGeoLocation->requestLocation();
}

void LXQtWeatherWidget::setupUI()
{
    // Создаем основной layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(4);

    // Иконка погоды
    mIconLabel = new QLabel();
    mIconLabel->setFixedSize(24, 24);
    mIconLabel->setAlignment(Qt::AlignCenter);
    mIconLabel->setPixmap(loadWeatherIcon("02d")); // Иконка по умолчанию
    mainLayout->addWidget(mIconLabel);

    // Вертикальный layout для текста
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(0);

    // Температура
    mTemperatureLabel = new QLabel("--°");
    mTemperatureLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mTemperatureLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    textLayout->addWidget(mTemperatureLabel);

    // Описание погоды
    mDescriptionLabel = new QLabel("");
    mDescriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mDescriptionLabel->setStyleSheet("font-size: 9px; color: #666;");
    mDescriptionLabel->setVisible(mShowDescription);
    textLayout->addWidget(mDescriptionLabel);

    mainLayout->addLayout(textLayout);

    setLayout(mainLayout);
    setFixedHeight(32);

    // Устанавливаем курсор-руку для индикации кликабельности
    setCursor(Qt::PointingHandCursor);

    qDebug() << "Weather widget UI setup completed";
}

void LXQtWeatherWidget::setupServices()
{
    // Создаем API клиент для Open-Meteo
    mWeatherAPI = new WeatherAPI(this);
    connect(mWeatherAPI, &WeatherAPI::weatherDataReceived,
            this, &LXQtWeatherWidget::onWeatherDataReceived);
    connect(mWeatherAPI, &WeatherAPI::errorOccurred,
            this, [this](const QString &error) {
                qWarning() << "Weather API error:" << error;
                if (mDescriptionLabel) {
                    mDescriptionLabel->setText("Error: " + error);
                }
            });

    // Создаем сервис геолокации
    mGeoLocation = new GeoLocation(this);
    // Подключаем сигналы геолокации
    connect(mGeoLocation, QOverload<double, double>::of(&GeoLocation::locationReceived),
            this, QOverload<double, double>::of(&LXQtWeatherWidget::onLocationReceived));
    connect(mGeoLocation, QOverload<const QString&>::of(&GeoLocation::locationReceived),
            this, QOverload<const QString&>::of(&LXQtWeatherWidget::onLocationReceived));
    connect(mGeoLocation, &GeoLocation::errorOccurred,
            this, [this](const QString &error) {
                qWarning() << "Geolocation error:" << error;
                // При ошибке геолокации используем Москву как fallback
                onLocationReceived(55.7558, 37.6176); // Москва
            });

    // Настраиваем таймер обновления (погода + геолокация)
    mUpdateTimer = new QTimer(this);
    mUpdateTimer->setInterval(mUpdateInterval * 60 * 1000); // Конвертируем минуты в миллисекунды
    connect(mUpdateTimer, &QTimer::timeout, this, &LXQtWeatherWidget::onUpdateTimer);
    mUpdateTimer->start();

    // Настраиваем мониторинг сетевых изменений
    mNetworkManager = new QNetworkConfigurationManager(this);
    connect(mNetworkManager, &QNetworkConfigurationManager::configurationChanged,
            this, &LXQtWeatherWidget::onNetworkConfigurationChanged);

    qDebug() << "Weather services initialized";
    qDebug() << "Update interval:" << mUpdateInterval << "minutes (weather + location)";
    qDebug() << "Network monitoring enabled";
}

void LXQtWeatherWidget::updateDisplay(const QString &description, const QString &iconCode)
{
    if (!mHasValidData) return;

    // Обновляем температуру с правильной единицей измерения
    if (mTemperatureLabel) {
        mTemperatureLabel->setText(formatTemperature(mCurrentTemperature));
    }

    // Обновляем описание погоды
    if (mDescriptionLabel && !description.isEmpty()) {
        mDescriptionLabel->setText(description);
        mDescriptionLabel->setVisible(mShowDescription);
    }

    // Обновляем иконку погоды
    if (mIconLabel && !iconCode.isEmpty()) {
        mIconLabel->setPixmap(loadWeatherIcon(iconCode));
    }

    update(); // Перерисовываем виджет (убирает "Loading...")

    qDebug() << "UI updated: temp =" << formatTemperature(mCurrentTemperature)
             << ", desc =" << description << ", icon =" << iconCode;
}

QString LXQtWeatherWidget::formatTemperature(double temperature) const
{
    if (mTemperatureUnit == "fahrenheit") {
        double fahrenheit = temperature * 9.0 / 5.0 + 32.0;
        return QString::number(qRound(fahrenheit)) + "°F";
    } else {
        return QString::number(qRound(temperature)) + "°C";
    }
}

QString LXQtWeatherWidget::getWeatherDescription(int weatherCode) const
{
    // Эта функция дублирует логику из WeatherAPI, но нужна для обратной совместимости
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
        case 61: return "Slight rain";
        case 63: return "Moderate rain";
        case 65: return "Heavy rain";
        case 71: return "Slight snow fall";
        case 73: return "Moderate snow fall";
        case 75: return "Heavy snow fall";
        case 80: return "Slight rain showers";
        case 85: return "Slight snow showers";
        case 95: return "Thunderstorm";
        default: return "Unknown";
    }
}

QPixmap LXQtWeatherWidget::loadWeatherIcon(const QString &iconCode) const
{
    // Создаем простые Unicode символы как иконки
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Arial", 16));
    painter.setPen(Qt::black);

    QString iconSymbol;
    if (iconCode.startsWith("01")) iconSymbol = "☀"; // Clear
    else if (iconCode.startsWith("02")) iconSymbol = "⛅"; // Few clouds
    else if (iconCode.startsWith("03")) iconSymbol = "☁"; // Scattered clouds
    else if (iconCode.startsWith("04")) iconSymbol = "☁"; // Broken clouds
    else if (iconCode.startsWith("09")) iconSymbol = "🌦"; // Shower rain
    else if (iconCode.startsWith("10")) iconSymbol = "🌧"; // Rain
    else if (iconCode.startsWith("11")) iconSymbol = "⛈"; // Thunderstorm
    else if (iconCode.startsWith("13")) iconSymbol = "🌨"; // Snow
    else if (iconCode.startsWith("50")) iconSymbol = "🌫"; // Mist
    else iconSymbol = "⛅"; // Default

    painter.drawText(pixmap.rect(), Qt::AlignCenter, iconSymbol);

    return pixmap;
}

void LXQtWeatherWidget::updateTooltip(const QJsonObject &data)
{
    QString tooltipText;

    // Используем реальное название города если доступно, иначе из данных
    QString locationName = mCurrentCity;
    if (locationName.isEmpty() && data.contains("name")) {
        locationName = data["name"].toString();
    }
    if (!locationName.isEmpty()) {
        tooltipText += "Location: " + locationName + "\n";
    }

    if (data.contains("main")) {
        QJsonObject main = data["main"].toObject();
        tooltipText += "Temperature: " + formatTemperature(main["temp"].toDouble()) + "\n";

        if (main.contains("humidity") && main["humidity"].toInt() > 0) {
            tooltipText += "Humidity: " + QString::number(main["humidity"].toInt()) + "%\n";
        }

        if (main.contains("pressure") && main["pressure"].toInt() > 0) {
            tooltipText += "Pressure: " + QString::number(main["pressure"].toInt()) + " hPa\n";
        }
    }

    if (data.contains("wind")) {
        QJsonObject wind = data["wind"].toObject();
        if (wind.contains("speed")) {
            tooltipText += "Wind: " + QString::number(wind["speed"].toDouble(), 'f', 1) + " m/s\n";
        }
    }

    if (data.contains("weather")) {
        QJsonArray weather = data["weather"].toArray();
        if (!weather.isEmpty()) {
            QJsonObject weatherInfo = weather[0].toObject();
            tooltipText += "Conditions: " + weatherInfo["description"].toString() + "\n";
        }
    }

    tooltipText += "\nLast updated: " + QDateTime::currentDateTime().toString("hh:mm:ss");
    tooltipText += "\nClick to refresh";

    setToolTip(tooltipText);
}
