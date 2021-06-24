#pragma once

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>
#include <optional>

QStringList SplitLines(const QString &_string);
std::pair<bool, std::optional<QString>> ValidateKernel(const QString &corePath, const QString &assetsPath);
