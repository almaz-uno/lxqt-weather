#include "lxqtweatherplugin.h"
#include "lxqtweatherwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>

// Simple adapter from PluginSettings to IWeatherSettings interface
class PluginSettingsAdapter : public IWeatherSettings
{
public:
    PluginSettingsAdapter(PluginSettings *settings) : mSettings(settings) {}

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const override {
        return mSettings ? mSettings->value(key, defaultValue) : defaultValue;
    }

    void setValue(const QString &key, const QVariant &value) override {
        if (mSettings) {
            mSettings->setValue(key, value);
        }
    }

    bool contains(const QString &key) const override {
        return mSettings ? mSettings->contains(key) : false;
    }

private:
    PluginSettings *mSettings;
};


LXQtWeatherPlugin::LXQtWeatherPlugin(const ILXQtPanelPluginStartupInfo &startupInfo)
    : QObject()
    , ILXQtPanelPlugin(startupInfo)
    , mWidget(nullptr)
{
    mWidget = new LXQtWeatherWidget();

    // Set default settings (no API key required)
    if (!settings()->contains("update_interval")) {
        settings()->setValue("update_interval", 30); // 30 minutes
    }
    if (!settings()->contains("temperature_unit")) {
        settings()->setValue("temperature_unit", "celsius");
    }
    if (!settings()->contains("show_description")) {
        settings()->setValue("show_description", true);
    }

    // Use adapter to pass settings to widget
    PluginSettingsAdapter adapter(settings());
    mWidget->updateSettings(&adapter);
}

LXQtWeatherPlugin::~LXQtWeatherPlugin()
{
    delete mWidget;
}

QWidget *LXQtWeatherPlugin::widget()
{
    return mWidget;
}

QDialog* LXQtWeatherPlugin::configureDialog()
{
    QDialog *dialog = new QDialog();
    dialog->setWindowTitle("Weather Widget Settings");

    // Calculate DPI-aware dialog size
    int dpi = dialog->logicalDpiX();
    double scaleFactor = dpi / 96.0;
    int dialogWidth = qRound(400 * scaleFactor);
    int dialogHeight = qRound(300 * scaleFactor);
    dialog->setMinimumSize(dialogWidth, dialogHeight);
    dialog->resize(dialogWidth, dialogHeight);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // Apply DPI-aware spacing and margins
    int layoutSpacing = qMax(6, qRound(9 * scaleFactor));
    int layoutMargin = qMax(9, qRound(12 * scaleFactor));
    mainLayout->setSpacing(layoutSpacing);
    mainLayout->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);

    // General settings group
    QGroupBox *generalGroup = new QGroupBox("General Settings");
    QFormLayout *generalLayout = new QFormLayout(generalGroup);

    // Apply DPI-aware font sizes and spacing
    int groupTitleFontSize = qMax(11, qRound(13 * scaleFactor));
    int labelFontSize = qMax(9, qRound(11 * scaleFactor));
    int controlFontSize = qMax(9, qRound(10 * scaleFactor));

    generalGroup->setStyleSheet(QString("QGroupBox::title { font-size: %1px; font-weight: bold; }").arg(groupTitleFontSize));
    generalLayout->setSpacing(qMax(6, qRound(8 * scaleFactor)));
    generalLayout->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);

    // Update interval (weather + geolocation)
    QSpinBox *updateIntervalSpinBox = new QSpinBox();
    updateIntervalSpinBox->setRange(5, 120); // From 5 to 120 minutes
    updateIntervalSpinBox->setSuffix(" minutes");
    updateIntervalSpinBox->setValue(settings()->value("update_interval", 30).toInt());
    updateIntervalSpinBox->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));

    QLabel *intervalLabel = new QLabel("Update Interval:");
    intervalLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));
    generalLayout->addRow(intervalLabel, updateIntervalSpinBox);

    // Temperature unit
    QComboBox *temperatureUnitComboBox = new QComboBox();
    temperatureUnitComboBox->addItem("Celsius (°C)", "celsius");
    temperatureUnitComboBox->addItem("Fahrenheit (°F)", "fahrenheit");
    temperatureUnitComboBox->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));

    QString currentUnit = settings()->value("temperature_unit", "celsius").toString();
    int unitIndex = temperatureUnitComboBox->findData(currentUnit);
    if (unitIndex >= 0) {
        temperatureUnitComboBox->setCurrentIndex(unitIndex);
    }

    QLabel *unitLabel = new QLabel("Temperature Unit:");
    unitLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));
    generalLayout->addRow(unitLabel, temperatureUnitComboBox);

    // Show weather description
    QCheckBox *showDescriptionCheckBox = new QCheckBox();
    showDescriptionCheckBox->setChecked(settings()->value("show_description", true).toBool());
    showDescriptionCheckBox->setStyleSheet(QString("font-size: %1px;").arg(controlFontSize));

    QLabel *descLabel = new QLabel("Show Weather Description:");
    descLabel->setStyleSheet(QString("font-size: %1px;").arg(labelFontSize));
    generalLayout->addRow(descLabel, showDescriptionCheckBox);

    mainLayout->addWidget(generalGroup);

    // Information group
    QGroupBox *infoGroup = new QGroupBox("Information");
    infoGroup->setStyleSheet(QString("QGroupBox::title { font-size: %1px; font-weight: bold; }").arg(groupTitleFontSize));

    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setContentsMargins(layoutMargin, layoutMargin, layoutMargin, layoutMargin);

    QLabel *infoLabel = new QLabel(
        "Weather data provided by Open-Meteo.com\n"
        "Open-Meteo is a free weather API that doesn't require registration.\n"
        "Location is determined automatically using your IP address.\n"
        "Location updates automatically when network changes (VPN, etc.)."
    );
    infoLabel->setWordWrap(true);

    // Apply DPI-aware font size to info label
    int infoFontSize = qMax(9, qRound(11 * scaleFactor));
    infoLabel->setStyleSheet(QString("color: #666; font-size: %1px;").arg(infoFontSize));
    infoLayout->addWidget(infoLabel);

    mainLayout->addWidget(infoGroup);

    mainLayout->addStretch();

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, dialog);

    // Apply DPI-aware font size to buttons
    int buttonFontSize = qMax(9, qRound(11 * scaleFactor));
    buttonBox->setStyleSheet(QString("QPushButton { font-size: %1px; }").arg(buttonFontSize));

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
        // Save settings
        settings()->setValue("update_interval", updateIntervalSpinBox->value());
        settings()->setValue("temperature_unit", temperatureUnitComboBox->currentData().toString());
        settings()->setValue("show_description", showDescriptionCheckBox->isChecked());

        // Update widget using adapter
        PluginSettingsAdapter adapter(settings());
        mWidget->updateSettings(&adapter);

        dialog->accept();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    // Debug: Print font sizes for settings dialog
    qDebug() << "Weather Settings Dialog - DPI:" << dpi << "Scale factor:" << scaleFactor
             << "Dialog size:" << dialogWidth << "x" << dialogHeight
             << "Font sizes: group=" << groupTitleFontSize << "px, label=" << labelFontSize
             << "px, control=" << controlFontSize << "px, info=" << infoFontSize << "px";

    return dialog;
}

void LXQtWeatherPlugin::settingsChanged()
{
    if (mWidget) {
        PluginSettingsAdapter adapter(settings());
        mWidget->updateSettings(&adapter);
    }
}
