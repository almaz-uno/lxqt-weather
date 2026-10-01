#ifndef RELNOTES_H
#define RELNOTES_H

#include <QString>

// The text of a GitHub release from RELEASE-NOTES.adoc (specs/001-releases;
// a port of qws's internal/relnotes)
namespace relnotes {

// The text of release version ("0.1.0", without "v") without its heading:
// for X.Y.0 the chapter "== X.Y.0 …" up to its first subsection or the next
// chapter — patches are releases of their own; for a patch the subsection
// "=== X.Y.Z …" up to the next heading. A missing section sets error.
QString extract(const QString &adoc, const QString &version, QString *error);

// Translates the markup of the notes: link:path[text] becomes a link to a
// file of the repository (base + path; the text may break over a line),
// "Label::" and *bold* become Markdown bold; the rest stays as it is — lists
// and `code` are the same in both.
QString markdown(const QString &text, const QString &base);

} // namespace relnotes

#endif // RELNOTES_H
