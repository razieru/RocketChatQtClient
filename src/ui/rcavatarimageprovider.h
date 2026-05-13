#pragma once

#include <QImage>
#include <QMutex>
#include <QtGlobal>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QSize>
#include <QString>
#include <QThreadPool>

#include "../network/apitypes.h"

namespace rc {

class RcAvatarImageResponse final : public QQuickImageResponse {
	Q_OBJECT

public:
	RcAvatarImageResponse(QString targetUserId, QSize requestedSize, SessionData session, QThreadPool* pool);

	void cancel() override;
	[[nodiscard]] QQuickTextureFactory* textureFactory() const override;

private:
	void load();

	QString m_targetUserId;
	QSize m_requestedSize;
	SessionData m_session;
	QAtomicInt m_cancelled{ 0 };
	QImage m_image;
};

class RcAvatarImageProvider final : public QQuickAsyncImageProvider {
public:
	RcAvatarImageProvider();

	void updateSession(const SessionData& session);
	void clearSession();

	[[nodiscard]] QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;

private:
	mutable QMutex m_mutex;
	SessionData m_session;
};

} // namespace rc
