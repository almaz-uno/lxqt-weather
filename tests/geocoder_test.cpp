// Criterion K1 of specs/002-location-by-address: the location setting and the
// geocoder, on the Nominatim responses recorded in research R1. The search is
// served by a local HTTP server on the loopback, never by the network

#include "geocoder.h"
#include "locationsetting.h"
#include "version.h"

#include <QFile>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QtTest>

namespace {

QByteArray fixture(const char *name)
{
    QFile file(QStringLiteral(FIXTURES_DIR "/") + QLatin1String(name));
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("no fixture %s", name);
    }
    return file.readAll();
}

class MemorySettings : public IWeatherSettings
{
public:
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const override {
        return mValues.value(key, defaultValue);
    }
    void setValue(const QString &key, const QVariant &value) override { mValues[key] = value; }
    bool contains(const QString &key) const override { return mValues.contains(key); }
    void remove(const QString &key) override { mValues.remove(key); }

    QMap<QString, QVariant> mValues;
};

// Answers every request with one body and remembers the last request's head
class FakeGeocoder : public QObject
{
public:
    explicit FakeGeocoder(const QByteArray &body) : mBody(body) {
        connect(&mServer, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket *socket = mServer.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                mBuffer += socket->readAll();
                if (!mBuffer.contains("\r\n\r\n")) {
                    return;
                }
                mHead = QString::fromUtf8(mBuffer.left(mBuffer.indexOf("\r\n\r\n")));
                mBuffer.clear();
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                              + QByteArray::number(mBody.size()) + "\r\nConnection: close\r\n\r\n" + mBody);
                socket->disconnectFromHost();
            });
        });
        mServer.listen(QHostAddress::LocalHost);
    }

    QString url() const { return QStringLiteral("http://127.0.0.1:%1").arg(mServer.serverPort()); }
    QString head() const { return mHead; }

private:
    QTcpServer mServer;
    QByteArray mBody;
    QByteArray mBuffer;
    QString mHead;
};

} // namespace

class GeocoderTest : public QObject
{
    Q_OBJECT

private slots:
    void coordinates_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("ok");
        QTest::addColumn<double>("latitude");
        QTest::addColumn<double>("longitude");
        QTest::newRow("comma") << "55.6928, 37.3475" << true << 55.6928 << 37.3475;
        QTest::newRow("space") << "55.6928 37.3475" << true << 55.6928 << 37.3475;
        QTest::newRow("signs") << "-33.92,18.42" << true << -33.92 << 18.42;
        QTest::newRow("padded") << " 55.6928 ,37.3475 " << true << 55.6928 << 37.3475;
        QTest::newRow("latitude out of range") << "91, 10" << false << 0.0 << 0.0;
        QTest::newRow("longitude out of range") << "10, 181" << false << 0.0 << 0.0;
        QTest::newRow("address") << "Технопарк Сколково" << false << 0.0 << 0.0;
        QTest::newRow("one number") << "55.6928" << false << 0.0 << 0.0;
        QTest::newRow("three numbers") << "55.6928, 37.3475, 100" << false << 0.0 << 0.0;
    }

    void coordinates()
    {
        QFETCH(QString, text);
        QFETCH(bool, ok);
        QFETCH(double, latitude);
        QFETCH(double, longitude);

        Place place;
        QCOMPARE(parseCoordinates(text, &place), ok);
        if (ok) {
            QCOMPARE(place.latitude, latitude);
            QCOMPARE(place.longitude, longitude);
            QCOMPARE(place.name, coordinatesName(latitude, longitude));
        }
    }

    void coordinatesNameRounds()
    {
        QCOMPARE(coordinatesName(55.69281, 37.34749), QStringLiteral("55.6928, 37.3475"));
    }

    void nominatimResponses_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<double>("latitude");
        QTest::addColumn<double>("longitude");
        QTest::addColumn<QString>("name");
        QTest::newRow("building") << "nominatim-found.json" << 55.6927882 << 37.3474601 << "Технопарк";
        QTest::newRow("street") << "nominatim-street.json" << 55.6980268 << 37.3563531 << "Большой бульвар";
    }

    void nominatimResponses()
    {
        QFETCH(QString, file);
        QFETCH(double, latitude);
        QFETCH(double, longitude);
        QFETCH(QString, name);

        Place place;
        QString error;
        QVERIFY2(parseNominatim(fixture(file.toUtf8().constData()), &place, &error), qPrintable(error));
        QCOMPARE(place.latitude, latitude);
        QCOMPARE(place.longitude, longitude);
        QCOMPARE(place.name, name);
    }

    void nominatimNotFound()
    {
        Place place;
        QString error;
        QVERIFY(!parseNominatim(fixture("nominatim-empty.json"), &place, &error));
        QCOMPARE(error, QStringLiteral("not found"));
    }

    void nominatimMalformed_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::newRow("truncated") << QByteArray("[{\"lat\":");
        QTest::newRow("object") << QByteArray("{\"error\":\"Unable to geocode\"}");
        QTest::newRow("no coordinates") << QByteArray("[{\"name\":\"x\"}]");
    }

    void nominatimMalformed()
    {
        QFETCH(QByteArray, json);
        Place place;
        QString error;
        QVERIFY(!parseNominatim(json, &place, &error));
        QCOMPARE(error, QStringLiteral("malformed geocoder response"));
    }

    void nominatimNameFromDisplayName()
    {
        Place place;
        QString error;
        QVERIFY(parseNominatim(R"([{"lat":"1.5","lon":"2.5","name":"","display_name":"A, B, C"}])", &place, &error));
        QCOMPARE(place.name, QStringLiteral("A"));
    }

    void request()
    {
        for (const QString &base : {QStringLiteral("https://nominatim.openstreetmap.org"),
                                    QStringLiteral("https://nominatim.openstreetmap.org/")}) {
            const QNetworkRequest request = nominatimRequest(base, QStringLiteral("Технопарк Сколково"));
            QCOMPARE(request.url().host(), QStringLiteral("nominatim.openstreetmap.org"));
            QCOMPARE(request.url().path(), QStringLiteral("/search"));
            const QUrlQuery query(request.url());
            QCOMPARE(query.queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded), QStringLiteral("Технопарк Сколково"));
            QCOMPARE(query.queryItemValue(QStringLiteral("format")), QStringLiteral("jsonv2"));
            QCOMPARE(query.queryItemValue(QStringLiteral("limit")), QStringLiteral("1"));
            QCOMPARE(request.header(QNetworkRequest::UserAgentHeader).toString(),
                     QStringLiteral("lxqt-weather/" LXQT_WEATHER_VERSION));
        }
        QCOMPARE(nominatimRequest(QStringLiteral("http://localhost:8080/nominatim"), QStringLiteral("x")).url().path(),
                 QStringLiteral("/nominatim/search"));
    }

    void settingCoordinates()
    {
        MemorySettings settings;
        LocationSetting location(&settings);
        QSignalSpy applied(&location, &LocationSetting::applied);

        location.apply(QStringLiteral("55.6928, 37.3475"));
        QCOMPARE(applied.count(), 1); // at once: coordinates need no search
        QCOMPARE(settings.value(LOCATION_KEY).toString(), QStringLiteral("55.6928, 37.3475"));
        QCOMPARE(settings.value(LOCATION_LATITUDE_KEY).toDouble(), 55.6928);
        QCOMPARE(settings.value(LOCATION_LONGITUDE_KEY).toDouble(), 37.3475);
        QCOMPARE(LocationSetting::describe(&settings), QStringLiteral("55.6928, 37.3475"));
    }

    void settingEmpty()
    {
        MemorySettings settings;
        settings.setValue(LOCATION_KEY, QStringLiteral("Технопарк Сколково"));
        settings.setValue(LOCATION_LATITUDE_KEY, 55.6927882);
        settings.setValue(LOCATION_LONGITUDE_KEY, 37.3474601);
        settings.setValue(LOCATION_NAME_KEY, QStringLiteral("Технопарк"));
        QCOMPARE(LocationSetting::describe(&settings), QStringLiteral("Технопарк (55.6928, 37.3475)"));

        LocationSetting location(&settings);
        QSignalSpy applied(&location, &LocationSetting::applied);
        location.apply(QStringLiteral("  "));
        QCOMPARE(applied.count(), 1);
        for (const char *key : {LOCATION_KEY, LOCATION_LATITUDE_KEY, LOCATION_LONGITUDE_KEY, LOCATION_NAME_KEY}) {
            QVERIFY2(!settings.contains(key), key);
        }
        QCOMPARE(LocationSetting::describe(&settings), QStringLiteral("Found by your IP address"));
    }

    void settingUnchangedIsNotSearched()
    {
        MemorySettings settings;
        settings.setValue(LOCATION_KEY, QStringLiteral("Технопарк Сколково"));
        settings.setValue(LOCATION_LATITUDE_KEY, 55.6927882);
        settings.setValue(LOCATION_LONGITUDE_KEY, 37.3474601);
        settings.setValue(LOCATION_NAME_KEY, QStringLiteral("Технопарк"));
        settings.setValue(GEOCODER_URL_KEY, QStringLiteral("http://127.0.0.1:9")); // would fail if asked

        LocationSetting location(&settings);
        QSignalSpy applied(&location, &LocationSetting::applied);
        QSignalSpy failed(&location, &LocationSetting::failed);
        location.apply(QStringLiteral("Технопарк Сколково"));
        QCOMPARE(applied.count(), 1); // at once, without a request
        QCOMPARE(failed.count(), 0);
    }

    void settingSearched()
    {
        FakeGeocoder geocoder(fixture("nominatim-found.json"));
        MemorySettings settings;
        settings.setValue(GEOCODER_URL_KEY, geocoder.url());

        LocationSetting location(&settings);
        QSignalSpy applied(&location, &LocationSetting::applied);
        location.apply(QStringLiteral("Технопарк Сколково"));
        QVERIFY(applied.wait(5000));

        QCOMPARE(settings.value(LOCATION_KEY).toString(), QStringLiteral("Технопарк Сколково"));
        QCOMPARE(settings.value(LOCATION_LATITUDE_KEY).toDouble(), 55.6927882);
        QCOMPARE(settings.value(LOCATION_LONGITUDE_KEY).toDouble(), 37.3474601);
        QCOMPARE(settings.value(LOCATION_NAME_KEY).toString(), QStringLiteral("Технопарк"));
        QVERIFY2(geocoder.head().startsWith(QStringLiteral("GET /search?")), qPrintable(geocoder.head()));
        QVERIFY2(geocoder.head().contains(QStringLiteral("User-Agent: lxqt-weather/" LXQT_WEATHER_VERSION)),
                 qPrintable(geocoder.head()));
    }

    void settingNotFoundKeepsTheLocation()
    {
        FakeGeocoder geocoder(fixture("nominatim-empty.json"));
        MemorySettings settings;
        settings.setValue(GEOCODER_URL_KEY, geocoder.url());
        settings.setValue(LOCATION_KEY, QStringLiteral("55.6928, 37.3475"));
        settings.setValue(LOCATION_LATITUDE_KEY, 55.6928);
        settings.setValue(LOCATION_LONGITUDE_KEY, 37.3475);
        settings.setValue(LOCATION_NAME_KEY, QStringLiteral("55.6928, 37.3475"));

        LocationSetting location(&settings);
        QSignalSpy failed(&location, &LocationSetting::failed);
        location.apply(QStringLiteral("Zzqxv Nonexistent 98765"));
        QVERIFY(failed.wait(5000));
        QCOMPARE(failed.first().first().toString(), QStringLiteral("not found"));
        QCOMPARE(settings.value(LOCATION_KEY).toString(), QStringLiteral("55.6928, 37.3475"));
        QCOMPARE(settings.value(LOCATION_LATITUDE_KEY).toDouble(), 55.6928);
    }
};

QTEST_GUILESS_MAIN(GeocoderTest)

#include "geocoder_test.moc"
