#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ui/authviewmodel.h"

int main(int argc, char* argv[]) {
	QGuiApplication app(argc, argv);
	qmlRegisterType<rc::AuthViewModel>("RocketChat.Client", 1, 0, "AuthViewModel");

	QQmlApplicationEngine engine;
	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("RocketChat.Client", "Main");

	return QGuiApplication::exec();
}
