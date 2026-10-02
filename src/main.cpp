#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QDebug>
#include <cstdio>

#include "lxqtweatherwidget.h"
#include "locationsetting.h"
#include "version.h"

    // Simple settings simulation for testing
class MockPluginSettings : public QObject, public IWeatherSettings
{
    Q_OBJECT

public:
    MockPluginSettings(QObject *parent = nullptr) : QObject(parent) {
        mSettings["update_interval"] = 30;
        mSettings["temperature_unit"] = "celsius";
        mSettings["show_description"] = true;
    }

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const override {
        return mSettings.value(key, defaultValue);
    }

    void setValue(const QString &key, const QVariant &value) override {
        mSettings[key] = value;
    }

    bool contains(const QString &key) const override {
        return mSettings.contains(key);
    }

    void remove(const QString &key) override {
        mSettings.remove(key);
    }

private:
    QMap<QString, QVariant> mSettings;
};

class TestWindow : public QMainWindow
{
    Q_OBJECT

public:
    // location: the text of the "Location" field to apply at start
    // (weather-test --location TEXT, specs/002-location-by-address)
    explicit TestWindow(const QString &location = QString(), QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setupUI();

        // Create settings
        mSettings = new MockPluginSettings(this);

        // Apply settings to widget
        mWeatherWidget->updateSettings(mSettings);

        // The location setting, as in the settings dialog of the panel
        mLocation = new LocationSetting(mSettings, this);
        connect(mLocation, &LocationSetting::applied, this, [this]() {
            mLocationResult->setText(LocationSetting::describe(mSettings));
            qDebug() << "Location:" << LocationSetting::describe(mSettings);
            mWeatherWidget->updateSettings(mSettings);
        });
        connect(mLocation, &LocationSetting::failed, this, [this](const QString &reason) {
            mLocationResult->setText(QString("Location not saved: %1").arg(reason));
            qWarning() << "Location not saved:" << reason;
        });
        mLocationResult->setText(LocationSetting::describe(mSettings));

        if (!location.isEmpty()) {
            mLocationEdit->setText(location);
            onLocationEntered();
        }
    }

private slots:
    void onRefreshClicked() {
        mWeatherWidget->refreshWeather();
    }

    void onLocationEntered() {
        mLocationResult->setText("Searching…");
        mLocation->apply(mLocationEdit->text());
    }

    void onSettingsChanged() {
        // Update settings
        mSettings->setValue("update_interval", mIntervalSpinBox->value());
        mSettings->setValue("temperature_unit", mUnitComboBox->currentData().toString());
        mSettings->setValue("show_description", mShowDescCheckBox->isChecked());

        // Apply to widget
        mWeatherWidget->updateSettings(mSettings);
    }

private:
    void setupUI() {
        setWindowTitle("LXQt Weather Widget Test - Open-Meteo API");

        // Calculate DPI-aware window size
        int dpi = logicalDpiX();
        double scaleFactor = dpi / 96.0;
        int windowWidth = qRound(400 * scaleFactor);
        int windowHeight = qRound(250 * scaleFactor); // Увеличиваем высоту
        setMinimumSize(windowWidth, windowHeight);
        resize(windowWidth, windowHeight); // Устанавливаем начальный размер

        QWidget *centralWidget = new QWidget();
        setCentralWidget(centralWidget);

        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // Calculate DPI-aware spacing and margins
        int layoutSpacing = qMax(4, qRound(6 * scaleFactor));
        int layoutMargin = qMax(6, qRound(9 * scaleFactor));
        mainLayout->setSpacing(layoutSpacing);
        mainLayout->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);

        // Weather widget
        QGroupBox *weatherGroup = new QGroupBox("Weather Widget");

        // Apply DPI-aware font size to group box title
        int groupTitleFontSize = qMax(10, qRound(12 * scaleFactor));
        weatherGroup->setStyleSheet(QString("QGroupBox::title { font-size: %1px; font-weight: bold; }").arg(groupTitleFontSize));

        QHBoxLayout *weatherLayout = new QHBoxLayout(weatherGroup);

        // Apply DPI-aware spacing to weather layout
        int groupSpacing = qMax(4, qRound(6 * scaleFactor));
        int groupMargin = qMax(4, qRound(6 * scaleFactor));
        weatherLayout->setSpacing(groupSpacing);
        weatherLayout->setContentsMargins(groupMargin, groupMargin, groupMargin, groupMargin);

        mWeatherWidget = new LXQtWeatherWidget();

        // Debug: Print widget size info
        qDebug() << "DPI:" << logicalDpiX() << "Scale factor:" << (logicalDpiX() / 96.0);
        qDebug() << "Widget size hint:" << mWeatherWidget->sizeHint();
        qDebug() << "Widget minimum size:" << mWeatherWidget->minimumSize();

        weatherLayout->addWidget(mWeatherWidget);

        QPushButton *refreshButton = new QPushButton("Refresh");

        // Apply DPI-aware font size to refresh button
        int buttonFontSize = qMax(10, qRound(11 * scaleFactor));
        refreshButton->setStyleSheet(QString("font-size: %1px;").arg(buttonFontSize));

        connect(refreshButton, &QPushButton::clicked, this, &TestWindow::onRefreshClicked);
        weatherLayout->addWidget(refreshButton);

        weatherLayout->addStretch();
        mainLayout->addWidget(weatherGroup);

        // Settings
        QGroupBox *settingsGroup = new QGroupBox("Settings");
        settingsGroup->setStyleSheet(QString("QGroupBox::title { font-size: %1px; font-weight: bold; }").arg(groupTitleFontSize));

        QFormLayout *settingsLayout = new QFormLayout(settingsGroup);

        // Apply DPI-aware spacing to settings layout
        settingsLayout->setSpacing(qMax(4, qRound(6 * scaleFactor)));
        settingsLayout->setContentsMargins(groupMargin, groupMargin, groupMargin, groupMargin);

        // Apply DPI-aware font size to form labels
        int labelFontSize = qMax(9, qRound(10 * scaleFactor));
        settingsLayout->setLabelAlignment(Qt::AlignLeft);

        // Create labels with DPI-aware font sizes
        QLabel *intervalLabel = new QLabel("Update Interval:");
        intervalLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));

        QLabel *unitLabel = new QLabel("Temperature Unit:");
        unitLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));

        QLabel *descLabel = new QLabel("Show Humidity and Pressure:");
        descLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));

        mIntervalSpinBox = new QSpinBox();
        mIntervalSpinBox->setRange(5, 120);
        mIntervalSpinBox->setValue(30);
        mIntervalSpinBox->setSuffix(" min");

        // Apply DPI-aware font size to spinbox
        int controlFontSize = qMax(9, qRound(10 * scaleFactor));
        mIntervalSpinBox->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));

        connect(mIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &TestWindow::onSettingsChanged);
        settingsLayout->addRow(intervalLabel, mIntervalSpinBox);

        mUnitComboBox = new QComboBox();
        mUnitComboBox->addItem("Celsius (°C)", "celsius");
        mUnitComboBox->addItem("Fahrenheit (°F)", "fahrenheit");
        mUnitComboBox->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));

        connect(mUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &TestWindow::onSettingsChanged);
        settingsLayout->addRow(unitLabel, mUnitComboBox);

        mShowDescCheckBox = new QCheckBox();
        mShowDescCheckBox->setChecked(true);
        mShowDescCheckBox->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));

        connect(mShowDescCheckBox, &QCheckBox::toggled,
                this, &TestWindow::onSettingsChanged);
        settingsLayout->addRow(descLabel, mShowDescCheckBox);

        // Location: applied on Enter or with the button, like OK in the panel
        QLabel *locationLabel = new QLabel("Location:");
        locationLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));

        mLocationEdit = new QLineEdit();
        mLocationEdit->setPlaceholderText("Address or latitude, longitude");
        mLocationEdit->setClearButtonEnabled(true);
        mLocationEdit->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));
        connect(mLocationEdit, &QLineEdit::returnPressed, this, &TestWindow::onLocationEntered);

        QPushButton *locationButton = new QPushButton("Apply");
        connect(locationButton, &QPushButton::clicked, this, &TestWindow::onLocationEntered);

        QHBoxLayout *locationLayout = new QHBoxLayout();
        locationLayout->addWidget(mLocationEdit);
        locationLayout->addWidget(locationButton);
        settingsLayout->addRow(locationLabel, locationLayout);

        mLocationResult = new QLabel();
        mLocationResult->setWordWrap(true);
        mLocationResult->setStyleSheet(QString("color: #666; font-size: %1px;").arg(labelFontSize));
        settingsLayout->addRow(QString(), mLocationResult);

        mainLayout->addWidget(settingsGroup);

        // Information
        QLabel *infoLabel = new QLabel(
            "Weather data provided by Open-Meteo.com (free, no API key required)\n"
            "Location: set above, or determined by your IP address\n"
            "Addresses are found by Nominatim, © OpenStreetMap contributors"
        );

        // Apply DPI-aware font size to info label
        int infoFontSize = qMax(9, qRound(11 * scaleFactor));
        infoLabel->setStyleSheet(QString("color: #666; font-size: %1px;").arg(infoFontSize));
        infoLabel->setWordWrap(true);
        mainLayout->addWidget(infoLabel);

        mainLayout->addStretch();

        // Debug: Print font sizes for all UI elements
        qDebug() << "Settings window DPI-aware fonts: group title=" << groupTitleFontSize
                 << "px, labels=" << labelFontSize << "px, controls=" << controlFontSize
                 << "px, button=" << buttonFontSize << "px, info=" << infoFontSize << "px";
    }

private:
    LXQtWeatherWidget *mWeatherWidget;
    MockPluginSettings *mSettings;

    QSpinBox *mIntervalSpinBox;
    QComboBox *mUnitComboBox;
    QCheckBox *mShowDescCheckBox;
    QLineEdit *mLocationEdit;
    QLabel *mLocationResult;
    LocationSetting *mLocation;
};

int main(int argc, char *argv[])
{
    // Before QApplication, so that it needs no display: the release script
    // checks the version of the build (specs/001-releases)
    if (argc == 2 && qstrcmp(argv[1], "--version") == 0) {
        std::printf("lxqt-weather %s\n", LXQT_WEATHER_VERSION);
        return 0;
    }

    QApplication app(argc, argv);

    // Set application information
    app.setApplicationName("LXQt Weather Widget Test");
    app.setApplicationVersion(LXQT_WEATHER_VERSION);
    app.setOrganizationName("LXQt");

    QString location;
    if (argc == 3 && qstrcmp(argv[1], "--location") == 0) {
        location = QString::fromLocal8Bit(argv[2]);
    }

    TestWindow window(location);
    window.show();

    return app.exec();
}

#include "main.moc"
