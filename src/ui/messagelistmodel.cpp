#include "messagelistmodel.h"

#include <QDate>
#include <QDateTime>
#include <QLocale>
#include <QTimeZone>

#include <utility>

namespace rc {

namespace {

QString formatMessageDateSection(qint64 timestampTicks) {
	if (timestampTicks <= 0) {
		return {};
	}

	const QDate date = QDateTime::fromSecsSinceEpoch(timestampTicks, QTimeZone::LocalTime).date();
	const QLocale locale = QLocale::system();
	if (date.year() == QDate::currentDate().year()) {
		return locale.toString(date, QStringLiteral("d MMMM"));
	}
	return locale.toString(date, QStringLiteral("d MMMM yyyy"));
}

} // namespace

MessageListModel::MessageListModel(QObject* parent) : QAbstractListModel(parent) {}

int MessageListModel::rowCount(const QModelIndex& parent) const {
	if (parent.isValid()) {
		return 0;
	}
	return m_messages.size();
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
		return {};
	}

	const MessageItem& message = m_messages.at(index.row());
	switch (role) {
	case IdRole:
		return message.id;
	case TextRole:
		return message.text;
	case AuthorRole:
		return message.author;
	case TimestampTicksRole:
		return message.timestampTicks;
	case DateSectionRole:
		return formatMessageDateSection(message.timestampTicks);
	default:
		return {};
	}
}

QHash<int, QByteArray> MessageListModel::roleNames() const {
	return {
		{ IdRole, "id" },
		{ TextRole, "text" },
		{ AuthorRole, "author" },
		{ TimestampTicksRole, "timestampTicks" },
		{ DateSectionRole, "dateSection" },
	};
}

void MessageListModel::setMessages(const QList<MessageItem>& messages) {
	QList<MessageItem> visible;
	visible.reserve(messages.size());
	for (const MessageItem& message : messages) {
		if (message.threadParentMessageId.isEmpty() || message.showInMainChannel) {
			visible.push_back(message);
		}
	}
	beginResetModel();
	m_messages = std::move(visible);
	endResetModel();
}

void MessageListModel::clear() {
	if (m_messages.isEmpty()) {
		return;
	}
	beginResetModel();
	m_messages.clear();
	endResetModel();
}

} // namespace rc
