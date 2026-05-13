#pragma once

#include <functional>

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QUrlQuery>

#include "apitypes.h"

namespace rc {

class HttpTransport : public QObject {
	Q_OBJECT

public:
	using SuccessHandler = std::function<void(const QJsonObject&)>;
	using RawSuccessHandler = std::function<void(const QByteArray&)>;
	using ErrorHandler = std::function<void(const ApiError&)>;
	using HeaderMap = QHash<QByteArray, QByteArray>;

	explicit HttpTransport(QObject* parent = nullptr);

	void setBaseUrl(const QUrl& baseUrl);
	void setCredentials(const QString& userId, const QString& authToken);
	void clearCredentials();
	[[nodiscard]] QUrl baseUrl() const;

	void get(const QString& endpoint, const QUrlQuery& query, const SuccessHandler& onSuccess, const ErrorHandler& onError);
	void getRaw(const QString& endpoint, const QUrlQuery& query, const RawSuccessHandler& onSuccess, const ErrorHandler& onError);
	void post(
		const QString& endpoint,
		const QJsonObject& payload,
		const SuccessHandler& onSuccess,
		const ErrorHandler& onError,
		const HeaderMap& extraHeaders = {});
	static void enrichTwoFactorError(ApiError& error, const QJsonObject& payload);

private:
	QNetworkRequest makeRequest(const QString& endpoint, const QUrlQuery& query = {}, const HeaderMap& extraHeaders = {}) const;
	[[nodiscard]] QNetworkRequest makeRawRequest(const QString& endpoint, const QUrlQuery& query = {}) const;
	void sendRequest(
		QNetworkReply* reply,
		const QString& endpoint,
		const SuccessHandler& onSuccess,
		const ErrorHandler& onError);
	void sendRawRequest(
		QNetworkReply* reply,
		const QString& endpoint,
		const RawSuccessHandler& onSuccess,
		const ErrorHandler& onError);

	QNetworkAccessManager m_networkAccessManager;
	QUrl m_baseUrl;
	QString m_userId;
	QString m_authToken;
	int m_timeoutMs = 15000;
};

} // namespace rc
