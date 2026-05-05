#pragma once

#include <memory>

#include <QJsonObject>
#include <QObject>

#include "../storage/isessionstore.h"
#include "apitypes.h"
#include "httptransport.h"

namespace rc {

class RocketChatClient : public QObject {
	Q_OBJECT

public:
	explicit RocketChatClient(std::unique_ptr<ISessionStore> sessionStore, QObject* parent = nullptr);

	void setServerUrl(const QUrl& serverUrl);
	[[nodiscard]] bool isAuthenticated() const;

	void login(const QString& user, const QString& password);
	void submitTwoFactorCode(const QString& code, const QString& method);
	void logout();
	void restoreSession();

	void getMe();
	void getRooms();
	void getSubscriptions();
	void getUserInfoByUsername(const QString& username);
	void getRoomMessages(const QString& roomId, const QString& roomType);

signals:
	void loginSucceeded();
	void loginFailed(const ApiError& error);
	void logoutFinished();
	void sessionRestored();
	void sessionInvalidated();
	void sessionChanged(bool authenticated);
	void twoFactorRequired(const QString& method, const QString& emailOrUsername, bool invalidAttempt);
	void meReceived(const QJsonObject& me);
	void roomsReceived(const QList<RoomInfo>& rooms);
	void roomsRequestFailed(const ApiError& error);
	void subscriptionsReceived(const QList<SubscriptionInfo>& subscriptions);
	void subscriptionsRequestFailed(const ApiError& error);
	void roomMessagesReceived(const QString& roomId, const QList<MessageInfo>& messages);
	void roomMessagesRequestFailed(const QString& roomId, const ApiError& error);
	void userInfoReceived(const QJsonObject& user);
	void userInfoRequestFailed(const QString& username);
	void requestFailed(const ApiError& error);

private:
	void applySession(const SessionData& session);
	void clearSession();
	void failRequest(const ApiError& error);
	void performLogin(const QString& user, const QString& password, const HttpTransport::HeaderMap& headers = {});
	void clearPendingLogin();
	void updateCurrentUsernameFromMePayload(const QJsonObject& payload);
	[[nodiscard]] QString apiPath(const QString& endpoint) const;

	HttpTransport m_transport;
	std::unique_ptr<ISessionStore> m_sessionStore;
	SessionData m_session;
	QString m_pendingLoginUser;
	QString m_pendingLoginPassword;
	QString m_currentUsername;
	bool m_hasPendingLogin = false;
};

} // namespace rc
