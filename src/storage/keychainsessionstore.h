#pragma once

#include <QSettings>

#include "isessionstore.h"

namespace rc {

class KeychainSessionStore : public ISessionStore {
public:
	explicit KeychainSessionStore(
		const QString& serviceName = QStringLiteral("RocketChatCpp.QtClient"),
		const QString& organization = QStringLiteral("RocketChatCpp"),
		const QString& application = QStringLiteral("QtClient"));

	bool save(const SessionData& session) override;
	SessionData load() const override;
	void clear() override;

	[[nodiscard]] bool isAvailable() const;

private:
	[[nodiscard]] bool writeSecret(const QString& key, const QString& value) const;
	[[nodiscard]] bool readSecret(const QString& key, QString& value) const;
	[[nodiscard]] bool deleteSecret(const QString& key) const;

	QString m_serviceName;
	mutable QSettings m_settings;
};

} // namespace rc
