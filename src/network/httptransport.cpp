#include "httptransport.h"

#include <QJsonDocument>
#include <QPointer>

namespace rc {

HttpTransport::HttpTransport(QObject* parent) : QObject(parent) {}

void HttpTransport::setBaseUrl(const QUrl& baseUrl) {
	m_baseUrl = baseUrl;
}

void HttpTransport::setCredentials(const QString& userId, const QString& authToken) {
	m_userId = userId;
	m_authToken = authToken;
}

void HttpTransport::clearCredentials() {
	m_userId.clear();
	m_authToken.clear();
}

QUrl HttpTransport::baseUrl() const {
	return m_baseUrl;
}

void HttpTransport::get(const QString& endpoint, const QUrlQuery& query, const SuccessHandler& onSuccess, const ErrorHandler& onError) {
	QNetworkRequest request = makeRequest(endpoint, query);
	sendRequest(m_networkAccessManager.get(request), endpoint, onSuccess, onError);
}

void HttpTransport::post(
	const QString& endpoint,
	const QJsonObject& payload,
	const SuccessHandler& onSuccess,
	const ErrorHandler& onError,
	const HeaderMap& extraHeaders) {
	QNetworkRequest request = makeRequest(endpoint, {}, extraHeaders);
	request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
	const QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
	sendRequest(m_networkAccessManager.post(request, body), endpoint, onSuccess, onError);
}

QNetworkRequest HttpTransport::makeRequest(const QString& endpoint, const QUrlQuery& query, const HeaderMap& extraHeaders) const {
	QUrl url(m_baseUrl.toString() + endpoint);
	if (!query.isEmpty()) {
		url.setQuery(query);
	}

	QNetworkRequest request(url);
	request.setRawHeader("Accept", "application/json");

	if (!m_authToken.isEmpty() && !m_userId.isEmpty()) {
		request.setRawHeader("X-Auth-Token", m_authToken.toUtf8());
		request.setRawHeader("X-User-Id", m_userId.toUtf8());
	}

	for (auto it = extraHeaders.constBegin(); it != extraHeaders.constEnd(); ++it) {
		request.setRawHeader(it.key(), it.value());
	}

	return request;
}

void HttpTransport::enrichTwoFactorError(ApiError& error, const QJsonObject& payload) {
	const QString errorType = payload.value(QStringLiteral("errorType")).toString();
	const QJsonObject details = payload.value(QStringLiteral("details")).toObject();
	const QString method = details.value(QStringLiteral("method")).toString();
	const QString emailOrUsername = details.value(QStringLiteral("emailOrUsername")).toString();

	error.twoFactorMethod = method;
	error.emailOrUsername = emailOrUsername;
	error.isTwoFactorRequired = errorType == QStringLiteral("totp-required");
	error.isTwoFactorInvalid = errorType == QStringLiteral("totp-invalid");
}

void HttpTransport::sendRequest(
	QNetworkReply* reply,
	const QString& endpoint,
	const SuccessHandler& onSuccess,
	const ErrorHandler& onError) {
	QPointer<QNetworkReply> safeReply(reply);
	QTimer* timer = new QTimer(reply);
	timer->setSingleShot(true);
	timer->start(m_timeoutMs);

	connect(timer, &QTimer::timeout, reply, [safeReply]() {
		if (safeReply) {
			safeReply->abort();
		}
	});

	connect(reply, &QNetworkReply::finished, this, [reply, endpoint, onSuccess, onError]() {
		const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const QByteArray bytes = reply->readAll();
		const QString body = QString::fromUtf8(bytes);

		if (reply->error() != QNetworkReply::NoError) {
			ApiError error{
				.kind = status > 0 ? ApiErrorKind::Http : ApiErrorKind::Network,
				.httpStatus = status,
				.endpoint = endpoint,
				.message = reply->errorString(),
				.rawBody = body,
			};

			QJsonParseError parseError;
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
			if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
				const QJsonObject object = doc.object();
				const QString responseError = object.value(QStringLiteral("error")).toString();
				if (!responseError.isEmpty()) {
					error.message = responseError;
				}
				HttpTransport::enrichTwoFactorError(error, object);
			}

			onError(error);
			reply->deleteLater();
			return;
		}

		QJsonParseError parseError;
		const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
		if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
			onError(ApiError{
				.kind = ApiErrorKind::Json,
				.httpStatus = status,
				.endpoint = endpoint,
				.message = parseError.errorString(),
				.rawBody = body,
			});
			reply->deleteLater();
			return;
		}

		onSuccess(doc.object());
		reply->deleteLater();
	});
}

} // namespace rc
