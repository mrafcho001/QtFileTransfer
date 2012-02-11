#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>

class MyTcpServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit MyTcpServer(int port, QObject *parent = 0);
	void startServer();
	
signals:
	void newConnectionDescriptor(int socket_descriptor);
	
public slots:

protected:
	void incomingConnection(int handle);

private:
	int portNumber;
	
};

#endif // MYTCPSERVER_H
