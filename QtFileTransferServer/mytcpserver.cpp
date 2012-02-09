#include "mytcpserver.h"
#include "../sharedstructures.h"

MyTcpServer::MyTcpServer(QObject *parent) :
	QTcpServer(parent)
{
}

void MyTcpServer::startServer()
{
	if(!this->listen(QHostAddress::Any, SERVER_LISTEN_PORT))
	{
		qDebug() << "Error creating listening server";
	}
	else
	{
		qDebug() << "Server started...";
	}
}

void MyTcpServer::incomingConnection(int handle)
{
	emit newConnectionDescriptor(handle);
}
