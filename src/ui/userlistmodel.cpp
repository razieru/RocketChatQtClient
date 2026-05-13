#include "userlistmodel.h"

namespace rc {

UserListModel::UserListModel(QObject* parent) : QAbstractListModel(parent) {}

int UserListModel::rowCount(const QModelIndex& parent) const {
	if (parent.isValid()) {
		return 0;
	}
	return m_users.size();
}

QVariant UserListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() < 0 || index.row() >= m_users.size()) {
		return {};
	}

	const UserListItem& user = m_users.at(index.row());
	switch (role) {
	case UserIdRole:
		return user.id;
	case UsernameRole:
		return user.username;
	case DisplayNameRole:
		return user.displayName;
	case StatusRole:
		return user.status;
	case ActiveRole:
		return user.active;
	case AvatarUrlRole:
		if (user.id.isEmpty()) {
			return QString();
		}
		return QStringLiteral("image://rcavatar/") + user.id;
	default:
		return {};
	}
}

QHash<int, QByteArray> UserListModel::roleNames() const {
	return {
		{ UserIdRole, "userId" },
		{ UsernameRole, "username" },
		{ DisplayNameRole, "displayName" },
		{ StatusRole, "status" },
		{ ActiveRole, "active" },
		{ AvatarUrlRole, "avatarUrl" },
	};
}

void UserListModel::setUsers(const QList<UserListItem>& users) {
	beginResetModel();
	m_users = users;
	endResetModel();
}

void UserListModel::clear() {
	if (m_users.isEmpty()) {
		return;
	}
	beginResetModel();
	m_users.clear();
	endResetModel();
}

int UserListModel::rowForUserId(const QString& userId) const {
	if (userId.isEmpty()) {
		return -1;
	}
	for (int i = 0; i < m_users.size(); ++i) {
		if (m_users.at(i).id == userId) {
			return i;
		}
	}
	return -1;
}

} // namespace rc
