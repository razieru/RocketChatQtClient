#pragma once

#include <memory>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QStringList>

#include "../network/rocketchatclient.h"
#include "chatlistmodel.h"
#include "messagelistmodel.h"

namespace rc {

class AuthViewModel : public QObject {
	Q_OBJECT
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
	Q_PROPERTY(QString selectedChatId READ selectedChatId NOTIFY selectedChatChanged)
	Q_PROPERTY(QString selectedChatType READ selectedChatType NOTIFY selectedChatChanged)
	Q_PROPERTY(QString selectedChatName READ selectedChatName NOTIFY selectedChatChanged)
	Q_PROPERTY(int selectedChatIndex READ selectedChatIndex NOTIFY selectedChatChanged)
	Q_PROPERTY(bool preferHumanReadableChatNames READ preferHumanReadableChatNames WRITE setPreferHumanReadableChatNames NOTIFY preferHumanReadableChatNamesChanged)

public:
	explicit AuthViewModel(QObject* parent = nullptr);

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
	[[nodiscard]] QString selectedChatId() const;
	[[nodiscard]] QString selectedChatType() const;
	[[nodiscard]] QString selectedChatName() const;
	[[nodiscard]] int selectedChatIndex() const;
	[[nodiscard]] bool preferHumanReadableChatNames() const;
	void setPreferHumanReadableChatNames(bool value);

	Q_INVOKABLE void login();
	Q_INVOKABLE void logout();
	Q_INVOKABLE void restoreSession();
	Q_INVOKABLE void submitTwoFactorCode();
	Q_INVOKABLE void submitTwoFactorCode(const QString& code, const QString& method);
	Q_INVOKABLE void reloadChats();
	Q_INVOKABLE void selectChat(int index);
	Q_INVOKABLE void reloadMessages();

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
	[[nodiscard]] static QString sectionNameForRoom(const RoomInfo& room);
	void applyFavoriteFlagsToRooms();
	void mergeDirectHumanNamesFromCache();
	void requestUserInfoForDirectPeers();
	void clearUserNameResolveState();
	void selectChatByIndex(int index);
	void setSelectedChat(const RoomInfo& room, int index);
	void clearSelectedChat();
	void ensureValidChatSelection();
	void setBusy(bool value);
	void setAuthenticated(bool value);
	void setErrorMessage(const QString& value);
	void setUserInfoJson(const QString& value);
	void resetTwoFactorState();

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
	QList<RoomInfo> m_lastRooms;
	QList<RoomInfo> m_displayedRooms;
	QSettings m_settings;
	bool m_preferHumanReadableChatNames = true;
	bool m_needsRoomsRefreshAfterMe = false;
	QHash<QString, bool> m_favoriteByRoomId;
	QHash<QString, QString> m_displayNameByUsername;
	QSet<QString> m_userInfoInFlight;
	QString m_selectedChatId;
	QString m_selectedChatType;
	QString m_selectedChatName;
	int m_selectedChatIndex = -1;
};

} // namespace rc
