#ifndef HELP_H
#define HELP_H

#include <QByteArray>


#define SAFE_DELETE(p) do {if (p) {delete (p); (p) = nullptr;}} while(0)

#ifndef HELP_JUST_HEAD_
QByteArray versionedShaderCode(const char *src);
#else
#include <QOpenGLContext>
static QByteArray versionedShaderCode(const char *src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
        versionedSrc.append(QByteArrayLiteral("#version 310 es\n"));
    else
        versionedSrc.append(QByteArrayLiteral("#version 330\n"));

    versionedSrc.append(src);
    return versionedSrc;
}
#endif //HELP_JUST_HEAD_

#endif // HELP_H
