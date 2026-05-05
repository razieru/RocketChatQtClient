#include "authviewmodel.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>

#include "../storage/settingspath.h"
#include "../storage/sessionstorefactory.h"

namespace rc {

AuthViewModel::AuthViewModel(QObject* parent) :
	QObject(parent), m_settings(settingsFilePath(), QSettings::IniFormat) {
	m_preferHumanReadableChatNames =
		m_settings.value(QStringLiteral("ui/chats/preferHumanReadableName"), true).toBool();

	SessionStoreBuildResult store = SessionStoreFactory::createDefault();
	m_client = std::make_unique<RocketChatClient>(std::move(store.store));
	m_client->setServerUrl(QUrl(m_serverUrl));

	connect(m_client.get(), &RocketChatClient::loginSucceeded, this, [this]() {
		setBusy(false);
		setAuthenticated(true);
		setErrorMessage(QString());
		setPassword(QString());
		emit passwordChanged();
		m_needsRoomsRefreshAfterMe = true;
		m_client->getMe();
		m_client->getRooms();
		m_client->getSubscriptions();
	});

	connect(m_client.get(), &RocketChatClient::loginFailed, this, [this](const ApiError& error) {
		setBusy(false);
		setAuthenticated(false);
		clearUserNameResolveState();
		setErrorMessage(error.message);
	});

	connect(m_client.get(), &RocketChatClient::requestFailed, this, [this](const ApiError& error) {
		setBusy(false);
		setErrorMessage(error.message);
	});

	connect(m_client.get(), &RocketChatClient::sessionChanged, this, [this](bool authenticated) {
		setAuthenticated(authenticated);
	});

	connect(m_client.get(), &RocketChatClient::sessionRestored, this, [this]() {
		setAuthenticated(true);
		setBusy(false);
		m_needsRoomsRefreshAfterMe = true;
		m_client->getMe();
		m_client->getRooms();
		m_client->getSubscriptions();
	});

	connect(m_client.get(), &RocketChatClient::sessionInvalidated, this, [this]() {
		setAuthenticated(false);
		setBusy(false);
		m_needsRoomsRefreshAfterMe = false;
		clearUserNameResolveState();
		m_favoriteByRoomId.clear();
		m_unreadByRoomId.clear();
		m_alertByRoomId.clear();
		m_hideUnreadStatusByRoomId.clear();
		m_tunreadByRoomId.clear();
		m_lastRooms.clear();
		m_displayedRooms.clear();
		m_chatsModel.clear();
		m_messagesModel.clear();
		clearSelectedChat();
	});

	connect(m_client.get(), &RocketChatClient::meReceived, this, [this](const QJsonObject& me) {
		updateSidebarPreferencesFromUserInfo(me);
		const QString prettyJson = QString::fromUtf8(QJsonDocument(me).toJson(QJsonDocument::Indented));
		setUserInfoJson(prettyJson);
		setErrorMessage(QString());
		rebuildChatsModel();
		ensureValidChatSelection();
		if (m_needsRoomsRefreshAfterMe && m_authenticated) {
			m_needsRoomsRefreshAfterMe = false;
			m_client->getRooms();
			m_client->getSubscriptions();
		}
	});

	connect(m_client.get(), &RocketChatClient::roomsReceived, this, [this](const QList<RoomInfo>& rooms) {
		m_lastRooms = rooms;
		applyFavoriteFlagsToRooms();
		for (RoomInfo& room : m_lastRooms) {
			if (m_unreadByRoomId.contains(room.id)) {
				room.unread = m_unreadByRoomId.value(room.id);
			}
			if (m_alertByRoomId.contains(room.id)) {
				room.alert = m_alertByRoomId.value(room.id);
			}
			if (m_hideUnreadStatusByRoomId.contains(room.id)) {
				room.hideUnreadStatus = m_hideUnreadStatusByRoomId.value(room.id);
			}
			if (m_tunreadByRoomId.contains(room.id)) {
				room.tunread = m_tunreadByRoomId.value(room.id);
			}
		}
		mergeDirectHumanNamesFromCache();
		rebuildChatsModel();
		ensureValidChatSelection();
		requestUserInfoForDirectPeers();
	});

	connect(m_client.get(), &RocketChatClient::subscriptionsReceived, this, [this](const QList<SubscriptionInfo>& subscriptions) {
		m_favoriteByRoomId.clear();
		m_unreadByRoomId.clear();
		m_alertByRoomId.clear();
		m_hideUnreadStatusByRoomId.clear();
		m_tunreadByRoomId.clear();
		for (const SubscriptionInfo& subscription : subscriptions) {
			if (subscription.roomId.isEmpty()) {
				continue;
			}
			m_favoriteByRoomId.insert(subscription.roomId, subscription.favorite);
			m_unreadByRoomId.insert(subscription.roomId, std::max(0, subscription.unread));
			m_alertByRoomId.insert(subscription.roomId, subscription.alert);
			m_hideUnreadStatusByRoomId.insert(subscription.roomId, subscription.hideUnreadStatus);
			m_tunreadByRoomId.insert(subscription.roomId, subscription.tunread);
		}
		int unreadRoomsCount = 0;
		for (const SubscriptionInfo& subscription : subscriptions) {
			RoomInfo probe;
			probe.unread = std::max(0, subscription.unread);
			probe.alert = subscription.alert;
			probe.hideUnreadStatus = subscription.hideUnreadStatus;
			probe.tunread = subscription.tunread;
			if (isUnreadRoom(probe)) {
				++unreadRoomsCount;
			}
        }
		applyFavoriteFlagsToRooms();
		for (RoomInfo& room : m_lastRooms) {
			if (m_unreadByRoomId.contains(room.id)) {
				room.unread = m_unreadByRoomId.value(room.id);
			}
			if (m_alertByRoomId.contains(room.id)) {
				room.alert = m_alertByRoomId.value(room.id);
			}
			if (m_hideUnreadStatusByRoomId.contains(room.id)) {
				room.hideUnreadStatus = m_hideUnreadStatusByRoomId.value(room.id);
			}
			if (m_tunreadByRoomId.contains(room.id)) {
				room.tunread = m_tunreadByRoomId.value(room.id);
			}
		}
		rebuildChatsModel();
		ensureValidChatSelection();
	});

	connect(m_client.get(), &RocketChatClient::subscriptionsRequestFailed, this, [this](const ApiError&) {
		// Keep chat list usable even if favorites cannot be refreshed right now.
	});

	connect(m_client.get(), &RocketChatClient::roomMessagesReceived, this, [this](const QString& roomId, const QList<MessageInfo>& messages) {
		if (roomId != m_selectedChatId) {
			return;
		}

		QList<MessageInfo> sorted = messages;
		std::sort(sorted.begin(), sorted.end(), [](const MessageInfo& left, const MessageInfo& right) {
			return left.timestampTicks < right.timestampTicks;
		});

		QList<MessageItem> items;
		items.reserve(sorted.size());
		for (const MessageInfo& message : sorted) {
			items.push_back(MessageItem{
				.id = message.id,
				.text = message.text,
				.author = message.authorUsername,
				.timestampTicks = message.timestampTicks,
			});
		}
		m_messagesModel.setMessages(items);
	});

	connect(m_client.get(), &RocketChatClient::roomMessagesRequestFailed, this, [this](const QString& roomId, const ApiError& error) {
		if (roomId != m_selectedChatId) {
			return;
		}
		m_messagesModel.clear();
		setErrorMessage(error.message);
	});

	connect(m_client.get(), &RocketChatClient::userInfoReceived, this, [this](const QJsonObject& user) {
		const QString apiUsername = user.value(QStringLiteral("username")).toString().trimmed();
		const QString name = user.value(QStringLiteral("name")).toString().trimmed();
		if (apiUsername.isEmpty()) {
			return;
		}
		const QString key = apiUsername.toLower();
		const QString label = name.isEmpty() ? apiUsername : name;
		m_displayNameByUsername.insert(key, label);
		m_userInfoInFlight.remove(key);
		mergeDirectHumanNamesFromCache();
		rebuildChatsModel();
		ensureValidChatSelection();
	});

	connect(m_client.get(), &RocketChatClient::userInfoRequestFailed, this, [this](const QString& username) {
		const QString key = username.trimmed().toLower();
		m_userInfoInFlight.remove(key);
		if (!key.isEmpty() && !m_displayNameByUsername.contains(key)) {
			m_displayNameByUsername.insert(key, username.trimmed());
		}
		mergeDirectHumanNamesFromCache();
		rebuildChatsModel();
		ensureValidChatSelection();
	});

	connect(m_client.get(), &RocketChatClient::roomsRequestFailed, this, [this](const ApiError& error) {
		clearUserNameResolveState();
		m_favoriteByRoomId.clear();
		m_lastRooms.clear();
		m_displayedRooms.clear();
		m_chatsModel.clear();
		m_messagesModel.clear();
		clearSelectedChat();
		setErrorMessage(error.message);
	});

	connect(m_client.get(), &RocketChatClient::twoFactorRequired, this, [this](const QString& method, const QString& account, bool invalidAttempt) {
		setBusy(false);
		m_twoFactorRequired = true;
		m_twoFactorMethod = method;
		m_twoFactorHint = account;
		m_twoFactorInvalid = invalidAttempt;
		emit twoFactorStateChanged();
	});
}

QString AuthViewModel::serverUrl() const {
	return m_serverUrl;
}

void AuthViewModel::setServerUrl(const QString& value) {
	if (m_serverUrl == value) {
		return;
	}
	m_serverUrl = value;
	m_client->setServerUrl(QUrl(m_serverUrl));
	emit serverUrlChanged();
}

QString AuthViewModel::username() const {
	return m_username;
}

void AuthViewModel::setUsername(const QString& value) {
	if (m_username == value) {
		return;
	}
	m_username = value;
	emit usernameChanged();
}

QString AuthViewModel::password() const {
	return m_password;
}

void AuthViewModel::setPassword(const QString& value) {
	if (m_password == value) {
		return;
	}
	m_password = value;
	emit passwordChanged();
}

bool AuthViewModel::busy() const {
	return m_busy;
}

bool AuthViewModel::authenticated() const {
	return m_authenticated;
}

QString AuthViewModel::errorMessage() const {
	return m_errorMessage;
}

QString AuthViewModel::userInfoJson() const {
	return m_userInfoJson;
}

bool AuthViewModel::twoFactorRequired() const {
	return m_twoFactorRequired;
}

QString AuthViewModel::twoFactorMethod() const {
	return m_twoFactorMethod;
}

QString AuthViewModel::twoFactorHint() const {
	return m_twoFactorHint;
}

bool AuthViewModel::twoFactorInvalid() const {
	return m_twoFactorInvalid;
}

QString AuthViewModel::twoFactorCode() const {
	return m_twoFactorCode;
}

QObject* AuthViewModel::chatsModel() const {
	return const_cast<ChatListModel*>(&m_chatsModel);
}

QObject* AuthViewModel::messagesModel() const {
	return const_cast<MessageListModel*>(&m_messagesModel);
}

QString AuthViewModel::selectedChatId() const {
	return m_selectedChatId;
}

QString AuthViewModel::selectedChatType() const {
	return m_selectedChatType;
}

QString AuthViewModel::selectedChatName() const {
	return m_selectedChatName;
}

int AuthViewModel::selectedChatIndex() const {
	return m_selectedChatIndex;
}

bool AuthViewModel::preferHumanReadableChatNames() const {
	return m_preferHumanReadableChatNames;
}

void AuthViewModel::setPreferHumanReadableChatNames(bool value) {
	if (m_preferHumanReadableChatNames == value) {
		return;
	}
	m_preferHumanReadableChatNames = value;
	m_settings.setValue(QStringLiteral("ui/chats/preferHumanReadableName"), value);
	m_settings.sync();
	mergeDirectHumanNamesFromCache();
	rebuildChatsModel();
	ensureValidChatSelection();
	if (value) {
		requestUserInfoForDirectPeers();
	}
	emit preferHumanReadableChatNamesChanged();
}

void AuthViewModel::setTwoFactorCode(const QString& value) {
	if (m_twoFactorCode == value) {
		return;
	}
	m_twoFactorCode = value;
	emit twoFactorCodeChanged();
}

void AuthViewModel::login() {
	if (m_username.isEmpty() || m_password.isEmpty() || m_serverUrl.isEmpty()) {
		setErrorMessage(QStringLiteral("Server URL, username and password are required."));
		return;
	}

	resetTwoFactorState();
	setBusy(true);
	setErrorMessage(QString());
	m_client->setServerUrl(QUrl(m_serverUrl));
	m_client->login(m_username, m_password);
}

void AuthViewModel::logout() {
	setBusy(true);
	m_client->logout();
	setBusy(false);
	setAuthenticated(false);
	resetTwoFactorState();
	setUserInfoJson(QString());
	m_needsRoomsRefreshAfterMe = false;
	clearUserNameResolveState();
	m_favoriteByRoomId.clear();
	m_unreadByRoomId.clear();
	m_alertByRoomId.clear();
	m_hideUnreadStatusByRoomId.clear();
	m_tunreadByRoomId.clear();
	m_lastRooms.clear();
	m_displayedRooms.clear();
	m_chatsModel.clear();
	m_messagesModel.clear();
	clearSelectedChat();
}

void AuthViewModel::restoreSession() {
	setBusy(true);
	m_client->setServerUrl(QUrl(m_serverUrl));
	m_client->restoreSession();
}

void AuthViewModel::submitTwoFactorCode() {
	submitTwoFactorCode(m_twoFactorCode, m_twoFactorMethod);
}

void AuthViewModel::submitTwoFactorCode(const QString& code, const QString& method) {
	if (code.isEmpty() || method.isEmpty()) {
		setErrorMessage(QStringLiteral("2FA code and method are required."));
		return;
	}
	setBusy(true);
	m_client->submitTwoFactorCode(code, method);
}

void AuthViewModel::reloadChats() {
	if (!m_authenticated) {
		m_chatsModel.clear();
		m_messagesModel.clear();
		clearSelectedChat();
		return;
	}
	m_client->getRooms();
	m_client->getSubscriptions();
}

void AuthViewModel::selectChat(int index) {
	selectChatByIndex(index);
}

void AuthViewModel::reloadMessages() {
	if (!m_authenticated || m_selectedChatId.isEmpty() || m_selectedChatType.isEmpty()) {
		m_messagesModel.clear();
		return;
	}
	m_client->getRoomMessages(m_selectedChatId, m_selectedChatType);
}

void AuthViewModel::rebuildChatsModel() {
	m_displayedRooms = m_lastRooms;

	auto roomSortLess = [](const RoomInfo& left, const RoomInfo& right) {
		const QString leftName = left.displayName.isEmpty() ? left.username : left.displayName;
		const QString rightName = right.displayName.isEmpty() ? right.username : right.displayName;
		const int byName = QString::compare(leftName, rightName, Qt::CaseInsensitive);
		if (byName != 0) {
			return byName < 0;
		}
		return left.id < right.id;
	};

	std::sort(m_displayedRooms.begin(), m_displayedRooms.end(), [this, roomSortLess](const RoomInfo& left, const RoomInfo& right) {
		const int leftSectionPriority = sectionPriorityForRoom(left);
		const int rightSectionPriority = sectionPriorityForRoom(right);
		if (leftSectionPriority != rightSectionPriority) {
			return leftSectionPriority < rightSectionPriority;
		}
		if (left.lastMessageTimestamp != right.lastMessageTimestamp) {
			return left.lastMessageTimestamp > right.lastMessageTimestamp;
		}
		return roomSortLess(left, right);
	});

	QList<ChatItem> chats;
	chats.reserve(m_displayedRooms.size());
	int unreadSectionCount = 0;
	for (const RoomInfo& room : m_displayedRooms) {
		const QString section = sectionNameForRoom(room);
		if (section == QStringLiteral("Unread")) {
			++unreadSectionCount;
		}
		const QString selectedName = m_preferHumanReadableChatNames ? room.displayName : room.username;
		chats.push_back(ChatItem{
			.id = room.id,
			.name = selectedName.isEmpty() ? room.displayName : selectedName,
			.displayName = room.displayName,
			.username = room.username,
			.type = room.type,
			.unread = room.unread,
			.section = section,
			.lastMessageTimestamp = room.lastMessageTimestamp,
		});
    }
	m_chatsModel.setChats(chats);
}

void AuthViewModel::selectChatByIndex(int index) {
	if (index < 0 || index >= m_displayedRooms.size()) {
		return;
	}
	setSelectedChat(m_displayedRooms.at(index), index);
	reloadMessages();
}

void AuthViewModel::setSelectedChat(const RoomInfo& room, int index) {
	const QString selectedName = m_preferHumanReadableChatNames ? room.displayName : room.username;
	QString nextName = selectedName.isEmpty() ? room.displayName : selectedName;
	if (nextName.isEmpty()) {
		nextName = room.id;
	}

	if (m_selectedChatId == room.id && m_selectedChatType == room.type && m_selectedChatName == nextName && m_selectedChatIndex == index) {
		return;
	}

	m_selectedChatId = room.id;
	m_selectedChatType = room.type;
	m_selectedChatName = nextName;
	m_selectedChatIndex = index;
	emit selectedChatChanged();
}

void AuthViewModel::clearSelectedChat() {
	if (m_selectedChatId.isEmpty() && m_selectedChatType.isEmpty() && m_selectedChatName.isEmpty() && m_selectedChatIndex == -1) {
		return;
	}
	m_selectedChatId.clear();
	m_selectedChatType.clear();
	m_selectedChatName.clear();
	m_selectedChatIndex = -1;
	emit selectedChatChanged();
}

void AuthViewModel::ensureValidChatSelection() {
	if (m_displayedRooms.isEmpty()) {
		clearSelectedChat();
		m_messagesModel.clear();
		return;
	}

	int selectedIndex = -1;
	if (!m_selectedChatId.isEmpty()) {
		for (int i = 0; i < m_displayedRooms.size(); ++i) {
			if (m_displayedRooms.at(i).id == m_selectedChatId) {
				selectedIndex = i;
				break;
			}
		}
	}

	if (selectedIndex < 0) {
		selectedIndex = 0;
	}

	const QString previousSelectedId = m_selectedChatId;
	setSelectedChat(m_displayedRooms.at(selectedIndex), selectedIndex);
	if (previousSelectedId != m_selectedChatId) {
		reloadMessages();
	}
}

QString AuthViewModel::sectionNameForRoom(const RoomInfo& room) const {
	if (m_sidebarShowUnread && isUnreadRoom(room)) {
		return QStringLiteral("Unread");
	}
	if (m_sidebarShowFavorites && room.favorite) {
		return QStringLiteral("Favorites");
	}
	if (!m_sidebarGroupByType) {
		return QString();
	}
	if (room.type == QStringLiteral("c")) {
		return QStringLiteral("Channels");
	}
	if (room.type == QStringLiteral("p")) {
		return QStringLiteral("Groups");
	}
	if (room.type == QStringLiteral("d")) {
		return QStringLiteral("Direct Messages");
	}
	return QStringLiteral("Other");
}

void AuthViewModel::updateSidebarPreferencesFromUserInfo(const QJsonObject& me) {
	const QJsonObject nestedMe = me.value(QStringLiteral("me")).toObject();
	const QJsonObject nestedPreferences =
		nestedMe.value(QStringLiteral("settings")).toObject().value(QStringLiteral("preferences")).toObject();
	const QJsonObject rootPreferences =
		me.value(QStringLiteral("settings")).toObject().value(QStringLiteral("preferences")).toObject();

	const auto resolvePreference = [&me, &nestedMe, &nestedPreferences, &rootPreferences](const QString& key, bool defaultValue) {
		if (nestedPreferences.contains(key)) {
			return nestedPreferences.value(key).toBool(defaultValue);
		}
		if (rootPreferences.contains(key)) {
			return rootPreferences.value(key).toBool(defaultValue);
		}
		const QString dottedKey = QStringLiteral("settings.preferences.") + key;
		if (nestedMe.contains(dottedKey)) {
			return nestedMe.value(dottedKey).toBool(defaultValue);
		}
		if (me.contains(dottedKey)) {
			return me.value(dottedKey).toBool(defaultValue);
		}
		return defaultValue;
	};

    m_sidebarGroupByType = resolvePreference(QStringLiteral("sidebarGroupByType"), false);
    m_sidebarShowFavorites = resolvePreference(QStringLiteral("sidebarShowFavorites"), false);
    m_sidebarShowUnread = resolvePreference(QStringLiteral("sidebarShowUnread"), false);
}

int AuthViewModel::sectionPriorityForRoom(const RoomInfo& room) const {
	if (m_sidebarShowUnread && isUnreadRoom(room)) {
		return 0;
	}
	if (m_sidebarShowFavorites && room.favorite) {
		return 1;
	}
	if (!m_sidebarGroupByType) {
		return 2;
	}
	if (room.type == QStringLiteral("c")) {
		return 2;
	}
	if (room.type == QStringLiteral("p")) {
		return 3;
	}
	if (room.type == QStringLiteral("d")) {
		return 4;
	}
	return 5;
}

bool AuthViewModel::isUnreadRoom(const RoomInfo& room) {
	if (room.hideUnreadStatus) {
		return false;
	}
	return room.alert || room.unread > 0 || !room.tunread.isEmpty();
}

void AuthViewModel::applyFavoriteFlagsToRooms() {
	for (RoomInfo& room : m_lastRooms) {
		room.favorite = m_favoriteByRoomId.value(room.id, false);
	}
}

void AuthViewModel::mergeDirectHumanNamesFromCache() {
	for (RoomInfo& room : m_lastRooms) {
		if (room.type != QStringLiteral("d") || room.directPeerUsernames.isEmpty()) {
			continue;
		}
		QStringList parts;
		parts.reserve(room.directPeerUsernames.size());
		for (const QString& peer : room.directPeerUsernames) {
			const QString key = peer.trimmed().toLower();
			const QString label = m_displayNameByUsername.value(key, peer);
			parts.push_back(label);
		}
		const QString joined = parts.join(QStringLiteral(", "));
		if (!joined.isEmpty()) {
			room.displayName = joined;
		}
	}
}

void AuthViewModel::requestUserInfoForDirectPeers() {
	if (!m_preferHumanReadableChatNames || !m_authenticated) {
		return;
	}
	QSet<QString> seen;
	for (const RoomInfo& room : m_lastRooms) {
		if (room.type != QStringLiteral("d")) {
			continue;
		}
		for (const QString& peer : room.directPeerUsernames) {
			const QString key = peer.trimmed().toLower();
			if (key.isEmpty() || seen.contains(key)) {
				continue;
			}
			seen.insert(key);
			if (m_displayNameByUsername.contains(key)) {
				continue;
			}
			if (m_userInfoInFlight.contains(key)) {
				continue;
			}
			m_userInfoInFlight.insert(key);
			m_client->getUserInfoByUsername(peer.trimmed());
		}
	}
}

void AuthViewModel::clearUserNameResolveState() {
	m_displayNameByUsername.clear();
	m_userInfoInFlight.clear();
}

void AuthViewModel::setBusy(bool value) {
	if (m_busy == value) {
		return;
	}
	m_busy = value;
	emit busyChanged();
}

void AuthViewModel::setAuthenticated(bool value) {
	if (m_authenticated == value) {
		return;
	}
	m_authenticated = value;
	emit authenticatedChanged();
}

void AuthViewModel::setErrorMessage(const QString& value) {
	if (m_errorMessage == value) {
		return;
	}
	m_errorMessage = value;
	emit errorMessageChanged();
}

void AuthViewModel::setUserInfoJson(const QString& value) {
	if (m_userInfoJson == value) {
		return;
	}
	m_userInfoJson = value;
	emit userInfoJsonChanged();
}

void AuthViewModel::resetTwoFactorState() {
	const bool changed = m_twoFactorRequired || m_twoFactorInvalid || !m_twoFactorMethod.isEmpty() || !m_twoFactorHint.isEmpty();
	m_twoFactorRequired = false;
	m_twoFactorInvalid = false;
	m_twoFactorMethod.clear();
	m_twoFactorHint.clear();
	m_twoFactorCode.clear();
	emit twoFactorCodeChanged();
	if (changed) {
		emit twoFactorStateChanged();
	}
}

} // namespace rc
