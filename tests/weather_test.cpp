// Criterion K1 of specs/005-humidity-pressure: the Open-Meteo request and the
// parsing of its response on the one recorded in research R1, the humidity
// and the pressure as the widget shows them. No network

#include "weatherapi.h"
#include "weatherformat.h"

#include <QFile>
#include <QJsonArray>
#include <QUrlQuery>
#include <QtTest>

class WeatherTest : public QObject
{
    Q_OBJECT

private slots:
    void parseRecordedResponse()
    {
        QFile file(QStringLiteral(FIXTURES_DIR "/open-meteo-current.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

        QJsonObject data;
        QString error;
        QVERIFY2(parseOpenMeteo(file.readAll(), &data, &error), qPrintable(error));

        const QJsonObject main = data["main"].toObject();
        QCOMPARE(main["temp"].toDouble(), 11.7);
        QCOMPARE(main["humidity"].toInt(), 61);
        QCOMPARE(main["pressure"].toDouble(), 1007.3);

        const QJsonObject wind = data["wind"].toObject();
        QCOMPARE(wind["speed"].toDouble(), 2.69); // m/s, not km/h
        QCOMPARE(wind["deg"].toDouble(), 312.0);

        const QJsonObject weather = data["weather"].toArray().first().toObject();
        QCOMPARE(weather["id"].toInt(), 3);
        QCOMPARE(weather["description"].toString(), QStringLiteral("Overcast"));
        QCOMPARE(weather["icon"].toString(), QStringLiteral("04d"));
    }

    void parseErrors_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("error");
        QTest::newRow("no block current") << QByteArray(R"({"latitude":55.6875,"current_weather":{}})")
                                         << "Invalid response format from Open-Meteo API";
        QTest::newRow("malformed") << QByteArray("{\"current\":")
                                   << "JSON parse error: unterminated object";
    }

    void parseErrors()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, error);
        QJsonObject data;
        QString got;
        QVERIFY(!parseOpenMeteo(json, &data, &got));
        QCOMPARE(got, error);
    }

    void pressure()
    {
        QCOMPARE(qRound(hpaToMmHg(1013.25) * 100) / 100.0, 760.0); // the standard atmosphere
        QCOMPARE(formatPressure(1007.3), QStringLiteral("756 mmHg"));
        QCOMPARE(formatPressure(1013.25), QStringLiteral("760 mmHg"));
        QCOMPARE(formatPressure(0.0), QString());
    }

    void details_data()
    {
        QTest::addColumn<int>("humidity");
        QTest::addColumn<double>("pressure");
        QTest::addColumn<QString>("line");
        QTest::newRow("both") << 61 << 1007.3 << "61% · 756 mmHg";
        QTest::newRow("humidity only") << 61 << 0.0 << "61%";
        QTest::newRow("pressure only") << 0 << 1007.3 << "756 mmHg";
        QTest::newRow("neither") << 0 << 0.0 << "";
    }

    void details()
    {
        QFETCH(int, humidity);
        QFETCH(double, pressure);
        QFETCH(QString, line);
        QCOMPARE(formatDetails(humidity, pressure), line);
    }

    void request()
    {
        const QUrl url = openMeteoUrl(55.692788, 37.347460);
        QCOMPARE(url.host(), QStringLiteral("api.open-meteo.com"));
        QCOMPARE(url.path(), QStringLiteral("/v1/forecast"));

        const QUrlQuery query(url);
        QCOMPARE(query.queryItemValue("latitude"), QStringLiteral("55.692788"));
        QCOMPARE(query.queryItemValue("longitude"), QStringLiteral("37.347460"));
        QCOMPARE(query.queryItemValue("current", QUrl::FullyDecoded),
                 QStringLiteral("temperature_2m,relative_humidity_2m,surface_pressure,"
                                "weather_code,wind_speed_10m,wind_direction_10m"));
        QCOMPARE(query.queryItemValue("wind_speed_unit"), QStringLiteral("ms"));
        for (const char *dropped : {"current_weather", "hourly", "daily"}) {
            QVERIFY2(!query.hasQueryItem(dropped), dropped);
        }
    }
};

QTEST_APPLESS_MAIN(WeatherTest)

#include "weather_test.moc"
