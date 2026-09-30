// Criterion K1 of specs/001-releases

#include "relnotes.h"

#include <QFile>
#include <QtTest>

class RelnotesTest : public QObject
{
    Q_OBJECT

private slots:
    // The section of the first release from the real notes
    void extractNotes()
    {
        QFile file(QStringLiteral(RELEASE_NOTES));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
        const QString adoc = QString::fromUtf8(file.readAll());

        QString error;
        const QString s = relnotes::extract(adoc, QStringLiteral("0.1.0"), &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY2(s != QLatin1String("\n") && !s.contains(QLatin1String("== ")), qPrintable(s));

        relnotes::extract(adoc, QStringLiteral("9.9.9"), &error);
        QVERIFY2(!error.isEmpty(), "no error for a missing section");
    }

    // A chapter is separated from its patches
    void extractLevels_data()
    {
        QTest::addColumn<QString>("version");
        QTest::addColumn<QString>("want");
        QTest::newRow("0.3.0") << "0.3.0" << "Three.\n";
        QTest::newRow("0.3.1") << "0.3.1" << "Three one.\n";
        QTest::newRow("0.3.2") << "0.3.2" << "Three two.\n";
        QTest::newRow("0.2.0") << "0.2.0" << "Two.\n";
    }

    void extractLevels()
    {
        QFETCH(QString, version);
        QFETCH(QString, want);
        const QString adoc = QStringLiteral(
            "= Notes\n\n== 0.3.0 — 2026-10-02\n\nThree.\n\n=== 0.3.1 — 2026-10-03\n\nThree one.\n\n"
            "=== 0.3.2 — 2026-10-04\n\nThree two.\n\n== 0.2.0 — 2026-10-01\n\nTwo.\n");

        QString error;
        QCOMPARE(relnotes::extract(adoc, version, &error), want);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }

    void markdownTranslation()
    {
        const QString in = QStringLiteral(
            "Versions — link:specs/001-releases/spec.adoc[specification\n001]; see "
            "link:https://example.org/x[x].\n\n"
            "*Compatibility.* The same keys.\n\n"
            "Fixed::\n* `weather-test --version` printed nothing; `*.deb` untouched.\n");
        const QString got = relnotes::markdown(
            in, QStringLiteral("https://github.com/almaz-uno/lxqt-weather/blob/v0.1.0/"));

        const QStringList wants = {
            QStringLiteral("[specification 001](https://github.com/almaz-uno/lxqt-weather/blob/v0.1.0/specs/001-releases/spec.adoc)"),
            QStringLiteral("[x](https://example.org/x)"),
            QStringLiteral("**Compatibility.** The same keys."),
            QStringLiteral("**Fixed**\n* `weather-test --version` printed nothing; `*.deb` untouched."),
        };
        for (const QString &want : wants) {
            QVERIFY2(got.contains(want), qPrintable(QStringLiteral("no %1 in\n%2").arg(want, got)));
        }
    }
};

QTEST_APPLESS_MAIN(RelnotesTest)

#include "relnotes_test.moc"
