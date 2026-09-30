#include "relnotes.h"

#include <QRegularExpression>
#include <QStringList>

namespace relnotes {

QString extract(const QString &adoc, const QString &version, QString *error)
{
    const QString level = version.endsWith(QLatin1String(".0"))
        ? QStringLiteral("== ") : QStringLiteral("=== ");
    const QString heading = level + version;
    const QStringList lines = adoc.split(QLatin1Char('\n'));

    int start = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i] == heading || lines[i].startsWith(heading + QLatin1Char(' '))) {
            start = i + 1;
            break;
        }
    }
    if (start < 0) {
        *error = QStringLiteral("no section \"%1\" in the release notes").arg(heading);
        return QString();
    }

    int end = lines.size();
    for (int i = start; i < lines.size(); ++i) {
        if (lines[i].startsWith(QLatin1String("== ")) || lines[i].startsWith(QLatin1String("=== "))) {
            end = i;
            break;
        }
    }

    error->clear();
    return lines.mid(start, end - start).join(QLatin1Char('\n')).trimmed() + QLatin1Char('\n');
}

QString markdown(const QString &text, const QString &base)
{
    static const QRegularExpression link(QStringLiteral("link:([^\\s\\[]+)\\[([^\\]]*)\\]"));
    static const QRegularExpression label(QStringLiteral("^(\\S[^\\n]*)::$"),
                                          QRegularExpression::MultilineOption);
    static const QRegularExpression strong(
        QStringLiteral("(^|[\\s(])\\*([^*\\s][^*\\n]*[^*\\s]|[^*\\s])\\*"));

    QString out;
    int last = 0;
    QRegularExpressionMatchIterator it = link.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        out += text.mid(last, m.capturedStart() - last);
        QString url = m.captured(1);
        if (!url.contains(QLatin1String("://"))) {
            url = base + url;
        }
        out += QLatin1Char('[') + m.captured(2).simplified() + QStringLiteral("](") + url + QLatin1Char(')');
        last = m.capturedEnd();
    }
    out += text.mid(last);

    out.replace(label, QStringLiteral("**\\1**"));
    out.replace(strong, QStringLiteral("\\1**\\2**"));
    return out;
}

} // namespace relnotes
