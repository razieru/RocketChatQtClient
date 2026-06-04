#include "rcavatarimageprovider.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQuickTextureFactory>
#include <QThreadPool>
#include <QTimer>
#include <QUrlQuery>
#include <utility>

namespace rc {

RcAvatarImageResponse::RcAvatarImageResponse(
	QString targetUserId,
	QSize requestedSize,
	SessionData session,
	QThreadPool* pool) :
	QQuickImageResponse(),
	m_targetUserId(std::move(targetUserId)),
	m_requestedSize(requestedSize),
	m_session(std::move(session)) {
	QThreadPool* usePool = pool ? pool : QThreadPool::globalInstance();
	usePool->start([this]() { load(); });
}

void RcAvatarImageResponse::cancel() {
	m_cancelled.storeRelaxed(1);
}

QQuickTextureFactory* RcAvatarImageResponse::textureFactory() const {
	return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void RcAvatarImageResponse::load() {
	QImage image;
	if (m_cancelled.loadRelaxed() == 0 && m_session.isValid() && !m_targetUserId.isEmpty()) {
		QUrl url(m_session.serverUrl.toString() + QStringLiteral("/api/v1/users.getAvatar"));
		QUrlQuery query;
		query.addQueryItem(QStringLiteral("userId"), m_targetUserId);
		url.setQuery(query);

		QNetworkRequest request(url);
		request.setRawHeader("Accept", "*/*");
		request.setRawHeader("X-Auth-Token", m_session.authToken.toUtf8());
		request.setRawHeader("X-User-Id", m_session.userId.toUtf8());

		QNetworkAccessManager nam;
		QNetworkReply* reply = nam.get(request);
		QEventLoop loop;
		QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		QTimer timer;
		timer.setSingleShot(true);
		QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		timer.start(15000);
		loop.exec();

		if (m_cancelled.loadRelaxed() == 0 && reply->error() == QNetworkReply::NoError) {
			const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			if (status >= 200 && status < 300) {
				const QByteArray data = reply->readAll();
				image = QImage::fromData(data);
			}
		}
		reply->deleteLater();
	}

	if (!m_requestedSize.isEmpty() && !image.isNull()) {
		image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	m_image = std::move(image);
	QMetaObject::invokeMethod(this, [this]() { emit finished(); }, Qt::QueuedConnection);
}

RcAvatarImageProvider::RcAvatarImageProvider() : QQuickAsyncImageProvider() {}

void RcAvatarImageProvider::updateSession(const SessionData& session) {
	QMutexLocker locker(&m_mutex);
	m_session = session;
}

void RcAvatarImageProvider::clearSession() {
	QMutexLocker locker(&m_mutex);
	m_session = SessionData{};
}

QQuickImageResponse* RcAvatarImageProvider::requestImageResponse(const QString& id, const QSize& requestedSize) {
	SessionData snap;
	{
		QMutexLocker locker(&m_mutex);
		snap = m_session;
	}
	return new RcAvatarImageResponse(id, requestedSize, snap, QThreadPool::globalInstance());
}

} // namespace rc
