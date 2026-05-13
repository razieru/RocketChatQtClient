#include "chatlistmodel.h"

namespace rc {

ChatListModel::ChatListModel(QObject* parent) : QAbstractListModel(parent) {}

int ChatListModel::rowCount(const QModelIndex& parent) const {
	if (parent.isValid()) {
		return 0;
	}
	return m_chats.size();
}

QVariant ChatListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() < 0 || index.row() >= m_chats.size()) {
		return {};
	}

	const ChatItem& chat = m_chats.at(index.row());
	switch (role) {
	case IdRole:
		return chat.id;
	case NameRole:
		return chat.name;
	case DisplayNameRole:
		return chat.displayName;
	case UsernameRole:
		return chat.username;
	case TypeRole:
		return chat.type;
	case UnreadRole:
		return chat.unread;
	case SectionRole:
		return chat.section;
	case LastMessageTimestampRole:
		return chat.lastMessageTimestamp;
	default:
		return {};
	}
}

QHash<int, QByteArray> ChatListModel::roleNames() const {
	return {
        { IdRole, "chatId" },
		{ NameRole, "name" },
		{ DisplayNameRole, "displayName" },
		{ UsernameRole, "username" },
		{ TypeRole, "type" },
		{ UnreadRole, "unread" },
		{ SectionRole, "section" },
		{ LastMessageTimestampRole, "lastMessageTimestamp" },
	};
}

void ChatListModel::setChats(const QList<ChatItem>& chats) {
	beginResetModel();
	m_chats = chats;
	endResetModel();
}

void ChatListModel::clear() {
	if (m_chats.isEmpty()) {
		return;
	}
	beginResetModel();
	m_chats.clear();
	endResetModel();
}

int ChatListModel::rowForChatId(const QString& chatId) const {
	if (chatId.isEmpty()) {
		return -1;
	}
	for (int i = 0; i < m_chats.size(); ++i) {
		if (m_chats.at(i).id == chatId) {
			return i;
		}
	}
	return -1;
}

} // namespace rc
