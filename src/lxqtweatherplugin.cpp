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
#include <QMessageBox>
#include <QDialogButtonBox>

// Простой адаптер для PluginSettings к интерфейсу IWeatherSettings
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

    // Устанавливаем настройки по умолчанию (без API ключа)
    if (!settings()->contains("update_interval")) {
        settings()->setValue("update_interval", 30); // 30 минут
    }
    if (!settings()->contains("temperature_unit")) {
        settings()->setValue("temperature_unit", "celsius");
    }
    if (!settings()->contains("show_description")) {
        settings()->setValue("show_description", true);
    }

    // Используем адаптер для передачи настроек виджету
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
    dialog->setFixedSize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // Группа общих настроек
    QGroupBox *generalGroup = new QGroupBox("General Settings");
    QFormLayout *generalLayout = new QFormLayout(generalGroup);

    // Интервал обновления
    QSpinBox *updateIntervalSpinBox = new QSpinBox();
    updateIntervalSpinBox->setRange(5, 120); // От 5 до 120 минут
    updateIntervalSpinBox->setSuffix(" minutes");
    updateIntervalSpinBox->setValue(settings()->value("update_interval", 30).toInt());
    generalLayout->addRow("Update Interval:", updateIntervalSpinBox);

    // Единица измерения температуры
    QComboBox *temperatureUnitComboBox = new QComboBox();
    temperatureUnitComboBox->addItem("Celsius (°C)", "celsius");
    temperatureUnitComboBox->addItem("Fahrenheit (°F)", "fahrenheit");

    QString currentUnit = settings()->value("temperature_unit", "celsius").toString();
    int unitIndex = temperatureUnitComboBox->findData(currentUnit);
    if (unitIndex >= 0) {
        temperatureUnitComboBox->setCurrentIndex(unitIndex);
    }
    generalLayout->addRow("Temperature Unit:", temperatureUnitComboBox);

    // Показывать описание погоды
    QCheckBox *showDescriptionCheckBox = new QCheckBox();
    showDescriptionCheckBox->setChecked(settings()->value("show_description", true).toBool());
    generalLayout->addRow("Show Weather Description:", showDescriptionCheckBox);

    mainLayout->addWidget(generalGroup);

    // Информационная группа
    QGroupBox *infoGroup = new QGroupBox("Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    QLabel *infoLabel = new QLabel(
        "Weather data provided by Open-Meteo.com\n"
        "Open-Meteo is a free weather API that doesn't require registration.\n"
        "Location is determined automatically using your IP address."
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666; font-size: 11px;");
    infoLayout->addWidget(infoLabel);

    mainLayout->addWidget(infoGroup);

    mainLayout->addStretch();

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, dialog);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
        // Сохраняем настройки
        settings()->setValue("update_interval", updateIntervalSpinBox->value());
        settings()->setValue("temperature_unit", temperatureUnitComboBox->currentData().toString());
        settings()->setValue("show_description", showDescriptionCheckBox->isChecked());

        // Обновляем виджет с помощью адаптера
        PluginSettingsAdapter adapter(settings());
        mWidget->updateSettings(&adapter);

        QMessageBox::information(dialog, "Settings Saved",
                               "Weather widget settings have been updated successfully!");
        dialog->accept();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    return dialog;
}

void LXQtWeatherPlugin::settingsChanged()
{
    if (mWidget) {
        PluginSettingsAdapter adapter(settings());
        mWidget->updateSettings(&adapter);
    }
}
