#include "rocketchatclient.h"

#include <algorithm>
#include <optional>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaType>
#include <QStringList>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace rc {

namespace {

UserListItem parseUserListItem(const QJsonObject& user) {
	UserListItem item;
	item.id = user.value(QStringLiteral("_id")).toString().trimmed();
	item.username = user.value(QStringLiteral("username")).toString().trimmed();
	item.displayName = user.value(QStringLiteral("name")).toString().trimmed();
	item.status = user.value(QStringLiteral("status")).toString().trimmed();
	item.active = user.value(QStringLiteral("active")).toBool(true);
	return item;
}

QString extractMessageIdFromLink(const QString& messageLink) {
	const QUrl url(messageLink);
	const QString fromQuery = QUrlQuery(url).queryItemValue(QStringLiteral("msg")).trimmed();
	if (!fromQuery.isEmpty()) {
		return fromQuery;
	}

	static const QRegularExpression pattern(QStringLiteral("[?&]msg=([^&#]+)"));
	const QRegularExpressionMatch match = pattern.match(messageLink);
	if (match.hasMatch()) {
		return QUrl::fromPercentEncoding(match.captured(1).toUtf8()).trimmed();
	}
	return {};
}

std::optional<QJsonObject> findFirstQuoteAttachmentObject(const QJsonArray& attachments) {
	for (const QJsonValue& entry : attachments) {
		if (!entry.isObject()) {
			continue;
		}
		const QJsonObject attachment = entry.toObject();
		if (!attachment.value(QStringLiteral("message_link")).isUndefined()) {
			return attachment;
		}
		const QJsonArray nested = attachment.value(QStringLiteral("attachments")).toArray();
		if (!nested.isEmpty()) {
			if (const std::optional<QJsonObject> nestedQuote = findFirstQuoteAttachmentObject(nested)) {
				return nestedQuote;
			}
		}
	}
	return std::nullopt;
}

void applyFirstQuoteAttachment(MessageInfo& info, const QJsonArray& attachments) {
	const std::optional<QJsonObject> quoteAttachment = findFirstQuoteAttachmentObject(attachments);
	if (!quoteAttachment.has_value()) {
		return;
	}

	const QJsonObject attachment = quoteAttachment.value();
	const QString messageLink = attachment.value(QStringLiteral("message_link")).toString().trimmed();
	if (!messageLink.isEmpty()) {
		info.quotedMessageId = extractMessageIdFromLink(messageLink);
	}

	QString body = attachment.value(QStringLiteral("text")).toString();
	const int newlineIndex = body.indexOf(QLatin1Char('\n'));
	if (newlineIndex >= 0) {
		body = body.mid(newlineIndex + 1);
	}
	body = body.trimmed();

	const QString authorName = attachment.value(QStringLiteral("author_name")).toString().trimmed();
	if (!authorName.isEmpty() && !body.isEmpty()) {
		info.quotePreviewText = authorName + QStringLiteral(": ") + body;
	} else if (!authorName.isEmpty()) {
		info.quotePreviewText = authorName;
	} else {
		info.quotePreviewText = body;
	}
}

} // namespace

RocketChatClient::RocketChatClient(std::unique_ptr<ISessionStore> sessionStore, QObject* parent) :
	QObject(parent), m_transport(this), m_sessionStore(std::move(sessionStore)) {
	qRegisterMetaType<UserListItem>("rc::UserListItem");
	qRegisterMetaType<QList<UserListItem>>("QList<rc::UserListItem>");
}

void RocketChatClient::setServerUrl(const QUrl& serverUrl) {
	m_session.serverUrl = serverUrl;
	m_transport.setBaseUrl(serverUrl);
}

bool RocketChatClient::isAuthenticated() const {
	return m_session.isValid();
}

SessionData RocketChatClient::session() const {
	return m_session;
}

void RocketChatClient::login(const QString& user, const QString& password) {
	m_pendingLoginUser = user;
	m_pendingLoginPassword = password;
	m_hasPendingLogin = true;
	performLogin(user, password);
}

void RocketChatClient::submitTwoFactorCode(const QString& code, const QString& method) {
	if (!m_hasPendingLogin) {
		failRequest(ApiError{
			.kind = ApiErrorKind::Protocol,
			.endpoint = apiPath(QStringLiteral("login")),
			.message = QStringLiteral("No pending login flow. Call login first."),
		});
		return;
	}

	if (code.isEmpty() || method.isEmpty()) {
		failRequest(ApiError{
			.kind = ApiErrorKind::Protocol,
			.endpoint = apiPath(QStringLiteral("login")),
			.message = QStringLiteral("2FA code and method must not be empty."),
		});
		return;
	}

	HttpTransport::HeaderMap headers;
	headers.insert("x-2fa-code", code.toUtf8());
	headers.insert("x-2fa-method", method.toUtf8());
	performLogin(m_pendingLoginUser, m_pendingLoginPassword, headers);
}

void RocketChatClient::performLogin(const QString& user, const QString& password, const HttpTransport::HeaderMap& headers) {
	if (!m_session.serverUrl.isValid()) {
		emit loginFailed(ApiError{
			.kind = ApiErrorKind::Protocol,
			.message = QStringLiteral("Server URL is not configured."),
		});
		clearPendingLogin();
		return;
	}

	m_transport.post(
		apiPath(QStringLiteral("login")),
		QJsonObject{
			{ QStringLiteral("user"), user },
			{ QStringLiteral("password"), password },
		},
		[this](const QJsonObject& json) {
			const QJsonObject data = json.value(QStringLiteral("data")).toObject();
			const QString token = data.value(QStringLiteral("authToken")).toString();
			const QString userId = data.value(QStringLiteral("userId")).toString();
			const QString tokenExpires = data.value(QStringLiteral("authTokenExpires")).toString();

			if (token.isEmpty() || userId.isEmpty()) {
				emit loginFailed(ApiError{
					.kind = ApiErrorKind::Protocol,
					.endpoint = apiPath(QStringLiteral("login")),
					.message = QStringLiteral("Login response does not contain authToken/userId."),
				});
				return;
			}

			SessionData next = m_session;
			next.authToken = token;
			next.userId = userId;
			next.tokenExpiresIso = tokenExpires;

			applySession(next);
			clearPendingLogin();
			emit loginSucceeded();
		},
		[this](const ApiError& error) {
			if (error.isTwoFactorRequired || error.isTwoFactorInvalid) {
				const QString method = error.twoFactorMethod.isEmpty() ? QStringLiteral("totp") : error.twoFactorMethod;
				emit twoFactorRequired(method, error.emailOrUsername, error.isTwoFactorInvalid);
				return;
			}
			clearPendingLogin();
			emit loginFailed(error);
		},
		headers);
}

void RocketChatClient::logout() {
	const QString endpoint = apiPath(QStringLiteral("logout"));
	m_transport.post(
		endpoint,
		QJsonObject{},
		[this](const QJsonObject&) {
			clearSession();
			emit logoutFinished();
		},
		[this](const ApiError&) {
			// Even if server-side logout fails, clear local credentials.
			clearSession();
			emit logoutFinished();
		});
}

void RocketChatClient::restoreSession() {
	const SessionData loaded = m_sessionStore->load();
	if (!loaded.isValid()) {
		emit sessionInvalidated();
		return;
	}

	applySession(loaded);
	m_transport.get(
		apiPath(QStringLiteral("me")),
		QUrlQuery{},
		[this](const QJsonObject& json) {
			updateCurrentUsernameFromMePayload(json);
			emit sessionRestored();
		},
		[this](const ApiError&) {
			clearSession();
			emit sessionInvalidated();
		});
}

void RocketChatClient::getMe() {
	if (!isAuthenticated()) {
		failRequest(ApiError{
			.kind = ApiErrorKind::NotAuthenticated,
			.endpoint = apiPath(QStringLiteral("me")),
			.message = QStringLiteral("Not authenticated."),
		});
		return;
	}

	m_transport.get(
		apiPath(QStringLiteral("me")),
		QUrlQuery{},
		[this](const QJsonObject& json) {
			updateCurrentUsernameFromMePayload(json);
			emit meReceived(json);
		},
		[this](const ApiError& error) { failRequest(error); });
}

void RocketChatClient::getRooms() {
	if (!isAuthenticated()) {
		const ApiError error{
			.kind = ApiErrorKind::NotAuthenticated,
			.endpoint = apiPath(QStringLiteral("rooms.get")),
			.message = QStringLiteral("Not authenticated."),
		};
		emit roomsRequestFailed(error);
		failRequest(error);
		return;
	}

	m_transport.get(
		apiPath(QStringLiteral("rooms.get")),
		QUrlQuery{},
		[this](const QJsonObject& json) {
			const auto normalizedString = [](const QJsonValue& value) -> QString {
				if (!value.isString()) {
					return {};
				}
				return value.toString().trimmed();
			};
			const auto normalizeTimestampToSeconds = [](const QJsonValue& value) -> qint64 {
				const auto secondsFromDateTime = [](const QDateTime& dt) -> qint64 {
					return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
				};

				if (value.isDouble()) {
					return static_cast<qint64>(value.toDouble() / 1000.0);
				}
				if (value.isString()) {
					const QDateTime parsed = QDateTime::fromString(value.toString().trimmed(), Qt::ISODate);
					return secondsFromDateTime(parsed);
				}
				if (value.isObject()) {
					const QJsonValue dateValue = value.toObject().value(QStringLiteral("$date"));
					if (dateValue.isDouble()) {
						return static_cast<qint64>(dateValue.toDouble() / 1000.0);
					}
					if (dateValue.isString()) {
						const QDateTime parsed = QDateTime::fromString(dateValue.toString().trimmed(), Qt::ISODate);
						return secondsFromDateTime(parsed);
					}
				}
				return 0;
			};

			const QJsonArray update = json.value(QStringLiteral("update")).toArray();
			QList<RoomInfo> rooms;
			rooms.reserve(update.size());

			for (const QJsonValue& entry : update) {
				const QJsonObject room = entry.toObject();
				if (room.isEmpty()) {
					continue;
				}

				RoomInfo info;
				info.id = normalizedString(room.value(QStringLiteral("_id")));
				info.type = normalizedString(room.value(QStringLiteral("t")));
				info.unread = room.value(QStringLiteral("unread")).toInt();
				info.favorite = room.value(QStringLiteral("f")).toBool(false);
				info.lastMessageTimestamp = normalizeTimestampToSeconds(room.value(QStringLiteral("lm")));
				info.username = normalizedString(room.value(QStringLiteral("name")));
				info.displayName.clear();

				if (info.type == QStringLiteral("d")) {
					const QJsonArray usernames = room.value(QStringLiteral("usernames")).toArray();
					QStringList directUsers;
					directUsers.reserve(usernames.size());
					for (const QJsonValue& userValue : usernames) {
						const QString username = normalizedString(userValue);
						if (!username.isEmpty() && !m_currentUsername.isEmpty() &&
							username.compare(m_currentUsername, Qt::CaseInsensitive) == 0) {
							continue;
						}
						if (!username.isEmpty()) {
							directUsers.push_back(username);
						}
					}
					if (!directUsers.isEmpty()) {
						info.displayName = directUsers.join(QStringLiteral(", "));
					}
					info.directPeerUsernames = directUsers;
				}

				if (info.displayName.isEmpty()) {
					info.displayName = normalizedString(room.value(QStringLiteral("fname")));
				}
				if (info.displayName.isEmpty()) {
					info.displayName = info.username;
				}
				if (info.displayName.isEmpty()) {
					info.displayName = info.id;
				}
				if (info.username.isEmpty()) {
					info.username = info.displayName;
				}
				if (info.id.isEmpty()) {
					continue;
				}
				rooms.push_back(info);
			}

			emit roomsReceived(rooms);
		},
		[this](const ApiError& error) {
			emit roomsRequestFailed(error);
			failRequest(error);
		});
}

void RocketChatClient::getSubscriptions() {
	if (!isAuthenticated()) {
		const ApiError error{
			.kind = ApiErrorKind::NotAuthenticated,
			.endpoint = apiPath(QStringLiteral("subscriptions.get")),
			.message = QStringLiteral("Not authenticated."),
		};
		emit subscriptionsRequestFailed(error);
		failRequest(error);
		return;
	}

	m_transport.get(
		apiPath(QStringLiteral("subscriptions.get")),
		QUrlQuery{},
		[this](const QJsonObject& json) {
			const QJsonArray update = json.value(QStringLiteral("update")).toArray();
			QList<SubscriptionInfo> subscriptions;
			subscriptions.reserve(update.size());

			for (const QJsonValue& entry : update) {
				const QJsonObject subscription = entry.toObject();
				if (subscription.isEmpty()) {
					continue;
				}

				SubscriptionInfo info;
				info.roomId = subscription.value(QStringLiteral("rid")).toString().trimmed();
				info.favorite = subscription.value(QStringLiteral("f")).toBool(false);
				info.unread = subscription.value(QStringLiteral("unread")).toInt(0);
				info.alert = subscription.value(QStringLiteral("alert")).toBool(false);
				info.hideUnreadStatus = subscription.value(QStringLiteral("hideUnreadStatus")).toBool(false);
				const QJsonArray tunreadArray = subscription.value(QStringLiteral("tunread")).toArray();
				info.tunread.reserve(tunreadArray.size());
				for (const QJsonValue& tunreadValue : tunreadArray) {
					const QString messageId = tunreadValue.toString().trimmed();
					if (!messageId.isEmpty()) {
						info.tunread.push_back(messageId);
					}
				}
				if (info.roomId.isEmpty()) {
					continue;
				}
				subscriptions.push_back(info);
			}

			emit subscriptionsReceived(subscriptions);
		},
		[this](const ApiError& error) {
			emit subscriptionsRequestFailed(error);
			failRequest(error);
		});
}

void RocketChatClient::getUserInfoByUsername(const QString& username) {
	if (!isAuthenticated()) {
		failRequest(ApiError{
			.kind = ApiErrorKind::NotAuthenticated,
			.endpoint = apiPath(QStringLiteral("users.info")),
			.message = QStringLiteral("Not authenticated."),
		});
		return;
	}

	QUrlQuery query;
	query.addQueryItem(QStringLiteral("username"), username);

	m_transport.get(
		apiPath(QStringLiteral("users.info")),
		query,
		[this, username](const QJsonObject& json) {
			const QJsonObject user = json.value(QStringLiteral("user")).toObject();
			if (user.isEmpty()) {
				emit userInfoRequestFailed(username);
				failRequest(ApiError{
					.kind = ApiErrorKind::Protocol,
					.endpoint = apiPath(QStringLiteral("users.info")),
					.message = QStringLiteral("Response does not contain 'user'."),
				});
				return;
			}
			emit userInfoReceived(user);
		},
		[this, username](const ApiError& error) {
			emit userInfoRequestFailed(username);
			failRequest(error);
		});
}

void RocketChatClient::listUsersPage(int offset, int count) {
	if (!isAuthenticated()) {
		const ApiError error{
			.kind = ApiErrorKind::NotAuthenticated,
			.endpoint = apiPath(QStringLiteral("users.list")),
			.message = QStringLiteral("Not authenticated."),
		};
		emit usersListRequestFailed(error);
		failRequest(error);
		return;
	}

	const int pageSize = std::clamp(count, 1, 100);
	const QJsonObject fields{
		{ QStringLiteral("_id"), 1 },
		{ QStringLiteral("username"), 1 },
		{ QStringLiteral("name"), 1 },
		{ QStringLiteral("status"), 1 },
		{ QStringLiteral("active"), 1 },
	};

	QUrlQuery query;
	query.addQueryItem(QStringLiteral("offset"), QString::number(offset));
	query.addQueryItem(QStringLiteral("count"), QString::number(pageSize));
	query.addQueryItem(
		QStringLiteral("fields"),
		QString::fromUtf8(QJsonDocument(fields).toJson(QJsonDocument::Compact)));

	m_transport.get(
		apiPath(QStringLiteral("users.list")),
		query,
		[this, offset](const QJsonObject& json) {
			const QJsonArray usersArray = json.value(QStringLiteral("users")).toArray();
			QList<UserListItem> users;
			users.reserve(usersArray.size());
			for (const QJsonValue& entry : usersArray) {
				const QJsonObject user = entry.toObject();
				if (user.isEmpty()) {
					continue;
				}
				UserListItem item = parseUserListItem(user);
				if (item.id.isEmpty()) {
					continue;
				}
				users.push_back(item);
			}
			const int total = json.value(QStringLiteral("total")).toInt(-1);
			emit usersListPageReceived(users, offset, total);
		},
		[this](const ApiError& error) {
			emit usersListRequestFailed(error);
			failRequest(error);
		});
}

void RocketChatClient::getRoomMessages(const QString& roomId, const QString& roomType) {
	if (!isAuthenticated()) {
		const ApiError error{
			.kind = ApiErrorKind::NotAuthenticated,
			.endpoint = apiPath(QStringLiteral("im.history")),
			.message = QStringLiteral("Not authenticated."),
		};
		emit roomMessagesRequestFailed(roomId, error);
		failRequest(error);
		return;
	}

	QString endpoint;
	if (roomType == QStringLiteral("d")) {
		endpoint = QStringLiteral("im.history");
	} else if (roomType == QStringLiteral("c")) {
		endpoint = QStringLiteral("channels.history");
	} else if (roomType == QStringLiteral("p")) {
		endpoint = QStringLiteral("groups.history");
	} else {
		const ApiError error{
			.kind = ApiErrorKind::Protocol,
			.endpoint = apiPath(QStringLiteral("im.history")),
			.message = QStringLiteral("Unsupported room type for history request."),
		};
		emit roomMessagesRequestFailed(roomId, error);
		failRequest(error);
		return;
	}

	QUrlQuery query;
	query.addQueryItem(QStringLiteral("roomId"), roomId);
	query.addQueryItem(QStringLiteral("count"), QStringLiteral("100"));

	m_transport.get(
		apiPath(endpoint),
		query,
		[this, roomId](const QJsonObject& json) {
			const auto normalizeTimestampToSeconds = [](const QJsonValue& value) -> qint64 {
				const auto secondsFromDateTime = [](const QDateTime& dt) -> qint64 {
					return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
				};

				if (value.isDouble()) {
					return static_cast<qint64>(value.toDouble() / 1000.0);
				}
				if (value.isString()) {
					const QDateTime parsed = QDateTime::fromString(value.toString().trimmed(), Qt::ISODate);
					return secondsFromDateTime(parsed);
				}
				if (value.isObject()) {
					const QJsonValue dateValue = value.toObject().value(QStringLiteral("$date"));
					if (dateValue.isDouble()) {
						return static_cast<qint64>(dateValue.toDouble() / 1000.0);
					}
					if (dateValue.isString()) {
						const QDateTime parsed = QDateTime::fromString(dateValue.toString().trimmed(), Qt::ISODate);
						return secondsFromDateTime(parsed);
					}
				}
				return 0;
			};

			const QJsonArray messagesArray = json.value(QStringLiteral("messages")).toArray();
			QList<MessageInfo> messages;
			messages.reserve(messagesArray.size());

			for (const QJsonValue& entry : messagesArray) {
				const QJsonObject message = entry.toObject();
				if (message.isEmpty()) {
					continue;
				}

				MessageInfo info;
				info.id = message.value(QStringLiteral("_id")).toString().trimmed();
				info.roomId = message.value(QStringLiteral("rid")).toString().trimmed();
				info.text = message.value(QStringLiteral("msg")).toString();
				info.timestampTicks = normalizeTimestampToSeconds(message.value(QStringLiteral("ts")));

				const QJsonObject user = message.value(QStringLiteral("u")).toObject();
				info.authorUsername = user.value(QStringLiteral("username")).toString().trimmed();
				info.threadParentMessageId = message.value(QStringLiteral("tmid")).toString().trimmed();
				info.showInMainChannel = message.value(QStringLiteral("tshow")).toBool(false);

				const QJsonArray attachments = message.value(QStringLiteral("attachments")).toArray();
				if (!attachments.isEmpty()) {
					applyFirstQuoteAttachment(info, attachments);
				}

				if (info.id.isEmpty()) {
					continue;
				}
				if (info.roomId.isEmpty()) {
					info.roomId = roomId;
				}
				messages.push_back(info);
			}

			emit roomMessagesReceived(roomId, messages);
		},
		[this, roomId](const ApiError& error) {
			emit roomMessagesRequestFailed(roomId, error);
			failRequest(error);
		});
}

void RocketChatClient::applySession(const SessionData& session) {
	m_session = session;
	m_transport.setBaseUrl(session.serverUrl);
	m_transport.setCredentials(session.userId, session.authToken);
	m_sessionStore->save(session);
	emit sessionChanged(true);
}

void RocketChatClient::clearSession() {
	clearPendingLogin();
	m_currentUsername.clear();
	m_session.authToken.clear();
	m_session.userId.clear();
	m_session.tokenExpiresIso.clear();
	m_transport.clearCredentials();
	m_sessionStore->clear();
	emit sessionChanged(false);
}

void RocketChatClient::failRequest(const ApiError& error) {
	emit requestFailed(error);
}

void RocketChatClient::clearPendingLogin() {
	m_pendingLoginUser.clear();
	m_pendingLoginPassword.clear();
	m_hasPendingLogin = false;
}

void RocketChatClient::updateCurrentUsernameFromMePayload(const QJsonObject& payload) {
	const QString username = payload.value(QStringLiteral("username")).toString().trimmed();
	if (!username.isEmpty()) {
		m_currentUsername = username;
		return;
	}

	const QJsonObject me = payload.value(QStringLiteral("me")).toObject();
	const QString nestedUsername = me.value(QStringLiteral("username")).toString().trimmed();
	if (!nestedUsername.isEmpty()) {
		m_currentUsername = nestedUsername;
	}
}

QString RocketChatClient::apiPath(const QString& endpoint) const {
	return QStringLiteral("/api/v1/") + endpoint;
}

} // namespace rc
