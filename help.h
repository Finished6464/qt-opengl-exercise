#ifndef HELP_H
#define HELP_H

#include <QByteArray>

#define SAFE_DELETE(p) do {if (p) {delete (p); (p) = nullptr;}} while(0)

QByteArray versionedShaderCode(const char *src);

#endif // HELP_H
