#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QtGlobal>

namespace rc {

struct ChatItem {
	QString id;
	QString name;
	QString displayName;
	QString username;
	QString type;
	int unread = 0;
	QString section;
	qint64 lastMessageTimestamp = 0;
};

class ChatListModel : public QAbstractListModel {
	Q_OBJECT

public:
	enum Roles {
		IdRole = Qt::UserRole + 1,
		NameRole,
		DisplayNameRole,
		UsernameRole,
		TypeRole,
		UnreadRole,
		SectionRole,
		LastMessageTimestampRole
	};
	Q_ENUM(Roles)

	explicit ChatListModel(QObject* parent = nullptr);

	[[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	[[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

	void setChats(const QList<ChatItem>& chats);
	void clear();

private:
	QList<ChatItem> m_chats;
};

} // namespace rc
