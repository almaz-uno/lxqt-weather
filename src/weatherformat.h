#ifndef WEATHERFORMAT_H
#define WEATHERFORMAT_H

#include <QString>

// The humidity and the pressure as the widget shows them
// (specs/005-humidity-pressure)

// Hectopascals to millimetres of mercury: 1 mmHg = 133.322387415 Pa
double hpaToMmHg(double hpa);

// "756 mmHg"; empty for a pressure that is not known (zero or less)
QString formatPressure(double hpa);

// "61%"; empty for a humidity that is not known (zero or less)
QString formatHumidity(int percent);

// The line under the temperature: "61% · 756 mmHg", or what of it is known
QString formatDetails(int humidityPercent, double pressureHpa);

#endif // WEATHERFORMAT_H
