// relnotes prints the text of a GitHub release from the release notes:
// relnotes <file.adoc> <version> <base of links> (specs/001-releases)

#include "relnotes.h"

#include <QFile>
#include <cstdio>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::fputs("usage: relnotes <RELEASE-NOTES.adoc> <X.Y.Z> <https://…/blob/<tag>/>\n", stderr);
        return 2;
    }

    QFile file(QString::fromLocal8Bit(argv[1]));
    if (!file.open(QIODevice::ReadOnly)) {
        std::fprintf(stderr, "%s: %s\n", argv[1], qPrintable(file.errorString()));
        return 1;
    }

    QString version = QString::fromUtf8(argv[2]);
    if (version.startsWith(QLatin1Char('v'))) {
        version.remove(0, 1);
    }

    QString error;
    const QString text = relnotes::extract(QString::fromUtf8(file.readAll()), version, &error);
    if (!error.isEmpty()) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        return 1;
    }

    const QByteArray out = relnotes::markdown(text, QString::fromUtf8(argv[3])).toUtf8();
    std::fwrite(out.constData(), 1, out.size(), stdout);
    return 0;
}
