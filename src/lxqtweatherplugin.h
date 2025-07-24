#ifndef LXQTWEATHERPLUGIN_H
#define LXQTWEATHERPLUGIN_H

#include <lxqt/ilxqtpanelplugin.h>
#include <lxqt/pluginsettings.h>
#include <lxqt/LXQt/lxqtsettings.h>

class LXQtWeatherWidget;

class LXQtWeatherPlugin : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT

public:
    explicit LXQtWeatherPlugin(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~LXQtWeatherPlugin() override;

    // ILXQtPanelPlugin interface
    QWidget *widget() override;
    QString themeId() const override { return QStringLiteral("WeatherPlugin"); }
    ILXQtPanelPlugin::Flags flags() const override { return HaveConfigDialog; }

    QDialog *configureDialog() override;
    void settingsChanged() override;

private:
    LXQtWeatherWidget *mWidget;
};

class LXQtWeatherPluginLibrary : public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)

public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new LXQtWeatherPlugin(startupInfo);
    }
};

#endif // LXQTWEATHERPLUGIN_H
