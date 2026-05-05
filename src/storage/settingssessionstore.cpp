#include "settingssessionstore.h"

#include "settingspath.h"

namespace rc {

SettingsSessionStore::SettingsSessionStore(const QString& organization, const QString& application) :
	m_settings(settingsFilePath(), QSettings::IniFormat) {
	Q_UNUSED(organization);
	Q_UNUSED(application);
}

bool SettingsSessionStore::save(const SessionData& session) {
	if (!session.isValid()) {
		return false;
	}

	m_settings.setValue(QStringLiteral("session/serverUrl"), session.serverUrl.toString());
	m_settings.setValue(QStringLiteral("session/userId"), session.userId);
	m_settings.setValue(QStringLiteral("session/authToken"), session.authToken);
	m_settings.setValue(QStringLiteral("session/tokenExpiresIso"), session.tokenExpiresIso);
	m_settings.sync();
	return m_settings.status() == QSettings::NoError;
}

SessionData SettingsSessionStore::load() const {
	SessionData data;
	data.serverUrl = QUrl(m_settings.value(QStringLiteral("session/serverUrl")).toString());
	data.userId = m_settings.value(QStringLiteral("session/userId")).toString();
	data.authToken = m_settings.value(QStringLiteral("session/authToken")).toString();
	data.tokenExpiresIso = m_settings.value(QStringLiteral("session/tokenExpiresIso")).toString();
	return data;
}

void SettingsSessionStore::clear() {
	m_settings.remove(QStringLiteral("session"));
	m_settings.sync();
}

} // namespace rc
