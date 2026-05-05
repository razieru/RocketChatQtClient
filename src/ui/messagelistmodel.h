#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QtGlobal>

namespace rc {

struct MessageItem {
	QString id;
	QString text;
	QString author;
	qint64 timestampTicks = 0;
};

class MessageListModel : public QAbstractListModel {
	Q_OBJECT

public:
	enum Roles {
		IdRole = Qt::UserRole + 1,
		TextRole,
		AuthorRole,
		TimestampTicksRole
	};
	Q_ENUM(Roles)

	explicit MessageListModel(QObject* parent = nullptr);

	[[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	[[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

	void setMessages(const QList<MessageItem>& messages);
	void clear();

private:
	QList<MessageItem> m_messages;
};

} // namespace rc
