#include "weatherformat.h"

#include <QStringList>
#include <QtMath>

double hpaToMmHg(double hpa)
{
    return hpa * 100.0 / 133.322387415;
}

QString formatPressure(double hpa)
{
    if (hpa <= 0.0) {
        return QString();
    }
    return QStringLiteral("%1 mmHg").arg(qRound(hpaToMmHg(hpa)));
}

QString formatHumidity(int percent)
{
    if (percent <= 0) {
        return QString();
    }
    return QStringLiteral("%1%").arg(percent);
}

QString formatDetails(int humidityPercent, double pressureHpa)
{
    QStringList parts;
    for (const QString &part : {formatHumidity(humidityPercent), formatPressure(pressureHpa)}) {
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }
    return parts.join(QStringLiteral(" · "));
}
