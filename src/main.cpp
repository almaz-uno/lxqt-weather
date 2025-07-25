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

#include "lxqtweatherwidget.h"

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

private:
    QMap<QString, QVariant> mSettings;
};

class TestWindow : public QMainWindow
{
    Q_OBJECT

public:
    TestWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setupUI();

        // Create settings
        mSettings = new MockPluginSettings(this);

        // Apply settings to widget
        mWeatherWidget->updateSettings(mSettings);
    }

private slots:
    void onRefreshClicked() {
        mWeatherWidget->refreshWeather();
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
        setFixedSize(400, 200);

        QWidget *centralWidget = new QWidget();
        setCentralWidget(centralWidget);

        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // Weather widget
        QGroupBox *weatherGroup = new QGroupBox("Weather Widget");
        QHBoxLayout *weatherLayout = new QHBoxLayout(weatherGroup);

        mWeatherWidget = new LXQtWeatherWidget();
        weatherLayout->addWidget(mWeatherWidget);

        QPushButton *refreshButton = new QPushButton("Refresh");
        connect(refreshButton, &QPushButton::clicked, this, &TestWindow::onRefreshClicked);
        weatherLayout->addWidget(refreshButton);

        weatherLayout->addStretch();
        mainLayout->addWidget(weatherGroup);

        // Settings
        QGroupBox *settingsGroup = new QGroupBox("Settings");
        QFormLayout *settingsLayout = new QFormLayout(settingsGroup);

        mIntervalSpinBox = new QSpinBox();
        mIntervalSpinBox->setRange(5, 120);
        mIntervalSpinBox->setValue(30);
        mIntervalSpinBox->setSuffix(" min");
        connect(mIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &TestWindow::onSettingsChanged);
        settingsLayout->addRow("Update Interval:", mIntervalSpinBox);

        mUnitComboBox = new QComboBox();
        mUnitComboBox->addItem("Celsius (°C)", "celsius");
        mUnitComboBox->addItem("Fahrenheit (°F)", "fahrenheit");
        connect(mUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &TestWindow::onSettingsChanged);
        settingsLayout->addRow("Temperature Unit:", mUnitComboBox);

        mShowDescCheckBox = new QCheckBox();
        mShowDescCheckBox->setChecked(true);
        connect(mShowDescCheckBox, &QCheckBox::toggled,
                this, &TestWindow::onSettingsChanged);
        settingsLayout->addRow("Show Description:", mShowDescCheckBox);

        mainLayout->addWidget(settingsGroup);

        // Information
        QLabel *infoLabel = new QLabel(
            "Weather data provided by Open-Meteo.com (free, no API key required)\n"
            "Location determined by IP geolocation service"
        );
        infoLabel->setStyleSheet("color: #666; font-size: 11px;");
        infoLabel->setWordWrap(true);
        mainLayout->addWidget(infoLabel);

        mainLayout->addStretch();
    }

private:
    LXQtWeatherWidget *mWeatherWidget;
    MockPluginSettings *mSettings;

    QSpinBox *mIntervalSpinBox;
    QComboBox *mUnitComboBox;
    QCheckBox *mShowDescCheckBox;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application information
    app.setApplicationName("LXQt Weather Widget Test");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("LXQt");

    TestWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
