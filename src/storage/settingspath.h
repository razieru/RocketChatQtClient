#pragma once

#include <QDir>
#include <QStandardPaths>
#include <QString>

namespace rc {

inline QString settingsFilePath() {
	const QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	const QString rootPath = basePath.isEmpty()
		? QStringLiteral(".")
		: basePath;
	const QString appFolder = rootPath + QStringLiteral("/RocketChatQtClient");
	QDir().mkpath(appFolder);
	return appFolder + QStringLiteral("/settings.ini");
}

} // namespace rc
