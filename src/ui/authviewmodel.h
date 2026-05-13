#pragma once

#include <memory>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <QStringList>

#include <QQmlEngine>
#include <QQmlParserStatus>

#include "../network/rocketchatclient.h"
#include "chatlistmodel.h"
#include "messagelistmodel.h"
#include "rcavatarimageprovider.h"
#include "userlistmodel.h"

namespace rc {

class AuthViewModel : public QObject, public QQmlParserStatus {
	Q_OBJECT
	Q_INTERFACES(QQmlParserStatus)
	Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
	Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
	Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
	Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
	Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
	Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
	Q_PROPERTY(QString userInfoJson READ userInfoJson NOTIFY userInfoJsonChanged)
	Q_PROPERTY(bool twoFactorRequired READ twoFactorRequired NOTIFY twoFactorStateChanged)
	Q_PROPERTY(QString twoFactorMethod READ twoFactorMethod NOTIFY twoFactorStateChanged)
	Q_PROPERTY(QString twoFactorHint READ twoFactorHint NOTIFY twoFactorStateChanged)
	Q_PROPERTY(bool twoFactorInvalid READ twoFactorInvalid NOTIFY twoFactorStateChanged)
	Q_PROPERTY(QString twoFactorCode READ twoFactorCode WRITE setTwoFactorCode NOTIFY twoFactorCodeChanged)
	Q_PROPERTY(QObject* chatsModel READ chatsModel CONSTANT)
	Q_PROPERTY(QObject* messagesModel READ messagesModel CONSTANT)
	Q_PROPERTY(QObject* usersModel READ usersModel CONSTANT)
	Q_PROPERTY(QString selectedChatId READ selectedChatId NOTIFY selectedChatChanged)
	Q_PROPERTY(QString selectedChatType READ selectedChatType NOTIFY selectedChatChanged)
	Q_PROPERTY(QString selectedChatName READ selectedChatName NOTIFY selectedChatChanged)
	Q_PROPERTY(bool preferHumanReadableChatNames READ preferHumanReadableChatNames WRITE setPreferHumanReadableChatNames NOTIFY preferHumanReadableChatNamesChanged)

public:
	explicit AuthViewModel(QObject* parent = nullptr);

	void classBegin() override {}
	void componentComplete() override;

	[[nodiscard]] QString serverUrl() const;
	void setServerUrl(const QString& value);

	[[nodiscard]] QString username() const;
	void setUsername(const QString& value);

	[[nodiscard]] QString password() const;
	void setPassword(const QString& value);

	[[nodiscard]] bool busy() const;
	[[nodiscard]] bool authenticated() const;
	[[nodiscard]] QString errorMessage() const;
	[[nodiscard]] QString userInfoJson() const;

	[[nodiscard]] bool twoFactorRequired() const;
	[[nodiscard]] QString twoFactorMethod() const;
	[[nodiscard]] QString twoFactorHint() const;
	[[nodiscard]] bool twoFactorInvalid() const;
	[[nodiscard]] QString twoFactorCode() const;
	void setTwoFactorCode(const QString& value);
	[[nodiscard]] QObject* chatsModel() const;
	[[nodiscard]] QObject* messagesModel() const;
	[[nodiscard]] QObject* usersModel() const;
	[[nodiscard]] QString selectedChatId() const;
	[[nodiscard]] QString selectedChatType() const;
	[[nodiscard]] QString selectedChatName() const;
	[[nodiscard]] bool preferHumanReadableChatNames() const;
	void setPreferHumanReadableChatNames(bool value);

	Q_INVOKABLE void login();
	Q_INVOKABLE void logout();
	Q_INVOKABLE void restoreSession();
	Q_INVOKABLE void submitTwoFactorCode();
	Q_INVOKABLE void submitTwoFactorCode(const QString& code, const QString& method);
	Q_INVOKABLE void reloadChats();
	Q_INVOKABLE void selectChat(const QString& chatId);
	Q_INVOKABLE void reloadMessages();
	Q_INVOKABLE void reloadUsers();

signals:
	void serverUrlChanged();
	void usernameChanged();
	void passwordChanged();
	void busyChanged();
	void authenticatedChanged();
	void errorMessageChanged();
	void userInfoJsonChanged();
	void twoFactorStateChanged();
	void twoFactorCodeChanged();
	void selectedChatChanged();
	void preferHumanReadableChatNamesChanged();

private:
	void rebuildChatsModel();
	[[nodiscard]] static bool isUnreadRoom(const RoomInfo& room);
	[[nodiscard]] QString sectionNameForRoom(const RoomInfo& room) const;
	void updateSidebarPreferencesFromUserInfo(const QJsonObject& me);
	[[nodiscard]] int sectionPriorityForRoom(const RoomInfo& room) const;
	void applyFavoriteFlagsToRooms();
	void mergeDirectHumanNamesFromCache();
	void requestUserInfoForDirectPeers();
	void clearUserNameResolveState();
	void setSelectedChat(const RoomInfo& room);
	void clearSelectedChat();
	void ensureValidChatSelection();
	void setBusy(bool value);
	void setAuthenticated(bool value);
	void setErrorMessage(const QString& value);
	void setUserInfoJson(const QString& value);
	void resetTwoFactorState();
	void ensureRcAvatarImageProviderRegistered();
	void syncAvatarProviderSession();
	void handleUsersListPage(const QList<UserListItem>& users, int offset, int total);

	std::unique_ptr<RocketChatClient> m_client;
	QString m_serverUrl = QStringLiteral("http://localhost:3000");
	QString m_username;
	QString m_password;
	bool m_busy = false;
	bool m_authenticated = false;
	QString m_errorMessage;
	QString m_userInfoJson;
	bool m_twoFactorRequired = false;
	QString m_twoFactorMethod;
	QString m_twoFactorHint;
	bool m_twoFactorInvalid = false;
	QString m_twoFactorCode;
	ChatListModel m_chatsModel;
	MessageListModel m_messagesModel;
	UserListModel m_usersModel;
	QPointer<RcAvatarImageProvider> m_avatarProvider;
	bool m_avatarProviderRegistered = false;
	QList<UserListItem> m_usersListAccum;
	bool m_usersListLoading = false;
	QList<RoomInfo> m_lastRooms;
	QList<RoomInfo> m_displayedRooms;
	QSettings m_settings;
	bool m_preferHumanReadableChatNames = true;
	bool m_needsRoomsRefreshAfterMe = false;
    bool m_sidebarGroupByType = false;
    bool m_sidebarShowFavorites = false;
	bool m_sidebarShowUnread = false;
	QHash<QString, bool> m_favoriteByRoomId;
	QHash<QString, int> m_unreadByRoomId;
	QHash<QString, bool> m_alertByRoomId;
	QHash<QString, bool> m_hideUnreadStatusByRoomId;
	QHash<QString, QStringList> m_tunreadByRoomId;
	QHash<QString, QString> m_displayNameByUsername;
	QSet<QString> m_userInfoInFlight;
	QString m_selectedChatId;
	QString m_selectedChatType;
	QString m_selectedChatName;
};

} // namespace rc
