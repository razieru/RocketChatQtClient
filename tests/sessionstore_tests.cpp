#include <QCoreApplication>
#include <QDateTime>
#include <QtTest/QtTest>

#include "../src/storage/sessionstorefactory.h"
#include "../src/storage/settingssessionstore.h"

namespace {

QString uniqueSuffix() {
	return QString::number(QDateTime::currentMSecsSinceEpoch());
}

rc::SessionData makeSessionData() {
	rc::SessionData data;
	data.serverUrl = QUrl(QStringLiteral("http://localhost:3000"));
	data.userId = QStringLiteral("u123");
	data.authToken = QStringLiteral("token123");
	data.tokenExpiresIso = QStringLiteral("2030-01-01T00:00:00.000Z");
	return data;
}

} // namespace

class SessionStoreTests : public QObject {
	Q_OBJECT

private slots:
	void settingsStore_SaveLoadClear();
	void defaultFactory_CreatesStore();
};

void SessionStoreTests::settingsStore_SaveLoadClear() {
	const QString suffix = uniqueSuffix();
	rc::SettingsSessionStore store(QStringLiteral("RocketChatCppTests"), QStringLiteral("QtClient_%1").arg(suffix));

	const rc::SessionData input = makeSessionData();
	QVERIFY(store.save(input));

	const rc::SessionData loaded = store.load();
	QCOMPARE(loaded.serverUrl, input.serverUrl);
	QCOMPARE(loaded.userId, input.userId);
	QCOMPARE(loaded.authToken, input.authToken);
	QCOMPARE(loaded.tokenExpiresIso, input.tokenExpiresIso);

	store.clear();
	const rc::SessionData cleared = store.load();
	QVERIFY(cleared.userId.isEmpty());
	QVERIFY(cleared.authToken.isEmpty());
}

void SessionStoreTests::defaultFactory_CreatesStore() {
	rc::SessionStoreBuildResult result = rc::SessionStoreFactory::createDefault();
	QVERIFY(result.store != nullptr);
}

QTEST_MAIN(SessionStoreTests)

#include "sessionstore_tests.moc"
