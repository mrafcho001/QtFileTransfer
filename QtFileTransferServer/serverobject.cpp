#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>

ServerObject::ServerObject(int sockDescriptor, QList<FileInfo> *file_list, QObject *parent) :
	QObject(parent), m_socketDescriptor(sockDescriptor), m_socket(NULL), m_file(NULL)
{
	m_fileList = file_list;
}

void ServerObject::handleConnection()
{
	qDebug() << "Handling connection...";
	m_socket = new QTcpSocket();

	if(!m_socket->setSocketDescriptor(m_socketDescriptor))
	{
		qDebug() << "QTcpSocket::setSocketDescriptor() failed, thread terminating...";
		return;
	}

	qDebug() << "Connected... State: " << m_socket->state();

	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readReady()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
}

void ServerObject::readReady()
{
	struct connControlMsg msg;

	if(m_socket->bytesAvailable() != sizeof(msg))
	{
		qDebug() << "Data is probably not a proper connControlMsg. Closing Connection.";
		m_socket->close();
	}

	m_socket->read((char*)&msg, sizeof(msg));

	m_bytes_sent = 0;
	switch(msg.message)
	{
	case CONN_CONTROL_REQUEST_FILE_LIST:
		m_currentMode = SENDING_LIST;
		m_items_sent = 0;
		if(m_fileList != NULL && m_fileList->count() > 0)
			m_socket->write((*m_fileList)[0].getByteArray());
		break;
	case CONN_CONTROL_REQUEST_FILE_DOWNLOAD:
		m_currentMode = SENDING_FILE;
		m_socket->write(QByteArray("File request acknowledged", 26));
		m_file_size = 26;
		break;
	case CONN_CONTROL_FILE_COMPLETE:
		m_socket->close();
		break;
	}
}

void ServerObject::bytesWritten(qint64 bytes)
{
	if(m_currentMode == SENDING_LIST)
	{
		m_items_sent++;
		if(m_items_sent < (unsigned int)m_fileList->count() )
		{
			qDebug() << "Sending liste item number " << m_items_sent;
			m_socket->write((*m_fileList)[m_items_sent].getByteArray());
		}
	}
	else
	{
		m_bytes_sent += bytes;
		qDebug() << "bytes: " << bytes;
		qDebug() << "m_bytes_sent: " << m_bytes_sent;
		qDebug() << "m_file_size: " << m_file_size;

		if(m_file_size <= m_bytes_sent)
		{
			//REMOVE ME
			m_socket->close();
		}
		else
		{
			m_socket->write("File List request acknowledged", 31);
		}
	}
}

void ServerObject::disconnected()
{
	qDebug() << "Socket clos(ed)(ing)";
	if(m_socket) m_socket->deleteLater();
	if(m_file) m_file->close();

	emit finished();
}
