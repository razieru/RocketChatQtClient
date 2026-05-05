#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QUrl>
#include <QMetaType>
#include <QtGlobal>

namespace rc {

struct SessionData {
	QUrl serverUrl;
	QString userId;
	QString authToken;
	QString tokenExpiresIso;

	[[nodiscard]] bool isValid() const {
		return serverUrl.isValid() && !userId.isEmpty() && !authToken.isEmpty();
	}
};

enum class ApiErrorKind {
	Network,
	Http,
	Json,
	Protocol,
	NotAuthenticated,
	Unknown,
};

struct ApiError {
	ApiErrorKind kind = ApiErrorKind::Unknown;
	int httpStatus = 0;
	QString endpoint;
	QString message;
	QString rawBody;
	bool isTwoFactorRequired = false;
	bool isTwoFactorInvalid = false;
	QString twoFactorMethod;
	QString emailOrUsername;
};

struct RoomInfo {
	QString id;
	QString displayName;
	QString username;
	QString type;
	int unread = 0;
	bool alert = false;
	bool hideUnreadStatus = false;
	QStringList tunread;
	bool favorite = false;
	qint64 lastMessageTimestamp = 0;
	/** For type "d": peer logins (excluding current user), for users.info resolution. */
	QStringList directPeerUsernames;
};

struct SubscriptionInfo {
	QString roomId;
	bool favorite = false;
	int unread = 0;
	bool alert = false;
	bool hideUnreadStatus = false;
	QStringList tunread;
};

struct MessageInfo {
	QString id;
	QString roomId;
	QString text;
	QString authorUsername;
	qint64 timestampTicks = 0;
};

} // namespace rc

Q_DECLARE_METATYPE(rc::ApiError)
Q_DECLARE_METATYPE(rc::RoomInfo)
Q_DECLARE_METATYPE(QList<rc::RoomInfo>)
Q_DECLARE_METATYPE(rc::SubscriptionInfo)
Q_DECLARE_METATYPE(QList<rc::SubscriptionInfo>)
Q_DECLARE_METATYPE(rc::MessageInfo)
Q_DECLARE_METATYPE(QList<rc::MessageInfo>)
