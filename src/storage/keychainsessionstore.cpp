#include "keychainsessionstore.h"

#ifdef RC_HAS_QTKEYCHAIN
#include <QEventLoop>
#if __has_include(<qt6keychain/keychain.h>)
#include <qt6keychain/keychain.h>
#elif __has_include(<qt5keychain/keychain.h>)
#include <qt5keychain/keychain.h>
#else
#include <keychain.h>
#endif
#endif

namespace rc {

namespace {
constexpr auto kSessionServerUrl = "session/serverUrl";
constexpr auto kSessionTokenExpiresIso = "session/tokenExpiresIso";
constexpr auto kSecretUserId = "session/userId";
constexpr auto kSecretAuthToken = "session/authToken";
} // namespace

KeychainSessionStore::KeychainSessionStore(const QString& serviceName, const QString& organization, const QString& application) :
	m_serviceName(serviceName), m_settings(organization, application) {}

bool KeychainSessionStore::save(const SessionData& session) {
	if (!session.isValid() || !isAvailable()) {
		return false;
	}

	if (!writeSecret(QLatin1String(kSecretUserId), session.userId)) {
		return false;
	}
	if (!writeSecret(QLatin1String(kSecretAuthToken), session.authToken)) {
		return false;
	}

	// Only non-sensitive metadata is persisted in QSettings.
	m_settings.setValue(QLatin1String(kSessionServerUrl), session.serverUrl.toString());
	m_settings.setValue(QLatin1String(kSessionTokenExpiresIso), session.tokenExpiresIso);
	m_settings.sync();
	return m_settings.status() == QSettings::NoError;
}

SessionData KeychainSessionStore::load() const {
	SessionData session;
	if (!isAvailable()) {
		return session;
	}

	session.serverUrl = QUrl(m_settings.value(QLatin1String(kSessionServerUrl)).toString());
	session.tokenExpiresIso = m_settings.value(QLatin1String(kSessionTokenExpiresIso)).toString();
	readSecret(QLatin1String(kSecretUserId), session.userId);
	readSecret(QLatin1String(kSecretAuthToken), session.authToken);
	return session;
}

void KeychainSessionStore::clear() {
	if (isAvailable()) {
		deleteSecret(QLatin1String(kSecretUserId));
		deleteSecret(QLatin1String(kSecretAuthToken));
	}
	m_settings.remove(QStringLiteral("session"));
	m_settings.sync();
}

bool KeychainSessionStore::isAvailable() const {
#ifdef RC_HAS_QTKEYCHAIN
	return true;
#else
	return false;
#endif
}

bool KeychainSessionStore::writeSecret(const QString& key, const QString& value) const {
#ifdef RC_HAS_QTKEYCHAIN
	QKeychain::WritePasswordJob job(m_serviceName);
	job.setAutoDelete(false);
	job.setKey(key);
	job.setTextData(value);

	QEventLoop loop;
	QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
	job.start();
	loop.exec();
	return job.error() == QKeychain::NoError;
#else
	Q_UNUSED(key);
	Q_UNUSED(value);
	return false;
#endif
}

bool KeychainSessionStore::readSecret(const QString& key, QString& value) const {
#ifdef RC_HAS_QTKEYCHAIN
	QKeychain::ReadPasswordJob job(m_serviceName);
	job.setAutoDelete(false);
	job.setKey(key);

	QEventLoop loop;
	QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
	job.start();
	loop.exec();

	if (job.error() != QKeychain::NoError) {
		return false;
	}

	value = job.textData();
	return true;
#else
	Q_UNUSED(key);
	Q_UNUSED(value);
	return false;
#endif
}

bool KeychainSessionStore::deleteSecret(const QString& key) const {
#ifdef RC_HAS_QTKEYCHAIN
	QKeychain::DeletePasswordJob job(m_serviceName);
	job.setAutoDelete(false);
	job.setKey(key);

	QEventLoop loop;
	QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
	job.start();
	loop.exec();

	return job.error() == QKeychain::NoError || job.error() == QKeychain::EntryNotFound;
#else
	Q_UNUSED(key);
	return false;
#endif
}

} // namespace rc
