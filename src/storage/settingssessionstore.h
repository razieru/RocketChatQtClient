#pragma once

#include <QSettings>

#include "isessionstore.h"

namespace rc {

class SettingsSessionStore : public ISessionStore {
public:
	explicit SettingsSessionStore(const QString& organization = QStringLiteral("RocketChatCpp"),
		const QString& application = QStringLiteral("QtClient"));

	bool save(const SessionData& session) override;
	SessionData load() const override;
	void clear() override;

private:
	QSettings m_settings;
};

} // namespace rc
