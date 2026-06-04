#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "../network/apitypes.h"

namespace rc {

class UserListModel : public QAbstractListModel {
	Q_OBJECT
	Q_PROPERTY(LoadStatus loadStatus READ loadStatus NOTIFY loadStatusChanged)

public:
	enum LoadStatus {
		NotLoaded = 0,
		Updating = 1,
		Updated = 2,
	};
	Q_ENUM(LoadStatus)

	enum Roles {
		UserIdRole = Qt::UserRole + 1,
		UsernameRole,
		DisplayNameRole,
		StatusRole,
		ActiveRole,
		AvatarUrlRole,
	};
	Q_ENUM(Roles)

	explicit UserListModel(QObject* parent = nullptr);

	[[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	[[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

	void setUsers(const QList<UserListItem>& users);
	void clear();

	void setLoadStatus(LoadStatus value);
	[[nodiscard]] LoadStatus loadStatus() const;

	Q_INVOKABLE int rowForUserId(const QString& userId) const;
	Q_INVOKABLE QString displayNameForUsername(const QString& username) const;

signals:
	void loadStatusChanged();

private:
	QList<UserListItem> m_users;
	LoadStatus m_loadStatus = NotLoaded;
};

} // namespace rc
