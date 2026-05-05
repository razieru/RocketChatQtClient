#include <QDateTime>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include "../src/network/httptransport.h"
#include "../src/network/rocketchatclient.h"
#include "../src/storage/settingssessionstore.h"

namespace {

std::unique_ptr<rc::ISessionStore> makeStore() {
	const QString suffix = QString::number(QDateTime::currentMSecsSinceEpoch());
	return std::make_unique<rc::SettingsSessionStore>(
		QStringLiteral("RocketChatCppTests"),
		QStringLiteral("TwoFa_%1").arg(suffix));
}

} // namespace

class TwoFaTests : public QObject {
	Q_OBJECT

private slots:
	void parseTwoFactorRequired();
	void parseTwoFactorInvalid();
	void submitTwoFactorWithoutPendingLogin();
};

void TwoFaTests::parseTwoFactorRequired() {
	rc::ApiError error;
	const QJsonObject payload{
		{ QStringLiteral("errorType"), QStringLiteral("totp-required") },
		{ QStringLiteral("details"),
			QJsonObject{
				{ QStringLiteral("method"), QStringLiteral("email") },
				{ QStringLiteral("emailOrUsername"), QStringLiteral("john@example.com") },
			} },
	};

	rc::HttpTransport::enrichTwoFactorError(error, payload);
	QVERIFY(error.isTwoFactorRequired);
	QVERIFY(!error.isTwoFactorInvalid);
	QCOMPARE(error.twoFactorMethod, QStringLiteral("email"));
	QCOMPARE(error.emailOrUsername, QStringLiteral("john@example.com"));
}

void TwoFaTests::parseTwoFactorInvalid() {
	rc::ApiError error;
	const QJsonObject payload{
		{ QStringLiteral("errorType"), QStringLiteral("totp-invalid") },
		{ QStringLiteral("details"),
			QJsonObject{
				{ QStringLiteral("method"), QStringLiteral("totp") },
			} },
	};

	rc::HttpTransport::enrichTwoFactorError(error, payload);
	QVERIFY(!error.isTwoFactorRequired);
	QVERIFY(error.isTwoFactorInvalid);
	QCOMPARE(error.twoFactorMethod, QStringLiteral("totp"));
}

void TwoFaTests::submitTwoFactorWithoutPendingLogin() {
	qRegisterMetaType<rc::ApiError>("rc::ApiError");
	rc::RocketChatClient client(makeStore());
	client.setServerUrl(QUrl(QStringLiteral("http://localhost:3000")));

	QSignalSpy failedSpy(&client, &rc::RocketChatClient::requestFailed);
	QVERIFY(failedSpy.isValid());

	client.submitTwoFactorCode(QStringLiteral("123456"), QStringLiteral("totp"));
	QCOMPARE(failedSpy.count(), 1);
}

QTEST_MAIN(TwoFaTests)

#include "twofa_tests.moc"
