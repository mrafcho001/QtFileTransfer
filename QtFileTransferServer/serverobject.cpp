#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>

ServerObject::ServerObject(int sockDescriptor, QList<FileInfo*> *file_list, QObject *parent) :
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
		qCritical() << "Data is probably not a proper connControlMsg. Closing Connection.";
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
			m_socket->write((*m_fileList)[0]->getByteArray());
		break;
	case CONN_CONTROL_REQUEST_FILE_DOWNLOAD:
	{
		m_currentMode = SENDING_FILE;

		QByteArray req_hash;
		//MORE MAGIC
		req_hash.append(msg.sha1_id, 20);
		for(int i = 0; i < m_fileList->count(); i++)
		{
			if(m_fileList->value(i)->getHash() == req_hash)
			{
				send_file = m_fileList->value(i);
				break;
			}
		}

		m_file = new QFile(send_file->getPath());
		m_file->open(QIODevice::ReadOnly);

		m_socket->write(m_file->read(FILE_CHUNK_SIZE));
	}
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
			qDebug() << "Sending " << m_fileList->value(m_items_sent)->getName()
					 << "; id: " << m_fileList->at(m_items_sent)->getSize()
					 << "; parentID: " << m_fileList->at(m_items_sent)->getParentId();
			m_socket->write((*m_fileList)[m_items_sent]->getByteArray());
		}
		else
			m_socket->close();
	}
	else
	{
		m_bytes_sent += bytes;
		if(m_bytes_sent == send_file->getSize())
			m_socket->close();
		else
		{
			m_socket->write(m_file->read(FILE_CHUNK_SIZE));
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
