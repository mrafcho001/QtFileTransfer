#include "mytcpserver.h"
#include "../sharedstructures.h"

MyTcpServer::MyTcpServer(int port, QObject *parent) :
	QTcpServer(parent), portNumber(port)
{
}

void MyTcpServer::startServer()
{
	if(!this->listen(QHostAddress::Any, portNumber))
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
