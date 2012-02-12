#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>
#include <QHostAddress>

ServerObject::ServerObject(int sockDescriptor, QList<FileInfo*> *file_list, QObject *parent) :
	QObject(parent), m_socketDescriptor(sockDescriptor), m_socket(NULL), m_file(NULL)
{
	m_fileList = file_list;
}

void ServerObject::handleConnection()
{
	m_socket = new QTcpSocket();

	if(!m_socket->setSocketDescriptor(m_socketDescriptor))
	{
		qCritical() << "QTcpSocket::setSocketDescriptor() failed, thread terminating...";
		return;
	}

	qDebug() << "Connected on port " << m_socket->localPort() << "... State: " << m_socket->state();

	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readReady()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	//connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
}

void ServerObject::readReady()
{
	struct connControlMsg msg;
	struct connControlMsg response;

	if(m_socket->bytesAvailable() != sizeof(msg))
	{
		qCritical() << "Data is probably not a proper connControlMsg. Closing Connection.";
		m_socket->close();
	}

	m_socket->read((char*)&msg, sizeof(msg));

	switch(msg.message)
	{
	case REQUEST_FILE_LIST:
	{
		m_currentMode = SENDING_LIST;
		m_items_sent = 0;
		if(m_fileList != NULL && m_fileList->count() > 0)
			response.message = LIST_REQUEST_GRANTED;
		else
			response.message = LIST_REQUEST_EMPTY;

		m_socket->write((char*)&response, sizeof(response));
		connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNextListItem(qint64)));
	}
		break;
	case REQUEST_FILE_DOWNLOAD:
	{
		m_currentMode = SENDING_FILE;
		m_file = NULL;

		QByteArray req_hash;
		req_hash.append(msg.sha1_id, SHA1_BYTECOUNT);
		for(int i = 0; i < m_fileList->count(); i++)
		{
			if(m_fileList->value(i)->getHash() == req_hash)
			{
				send_file = m_fileList->value(i);
				break;
			}
		}

		if(send_file == NULL)
			response.message = FILE_DOWNLOAD_REQUEST_REJECTED;
		else
		{
			response.message = FILE_DOWNLOAD_REQUEST_GRANTED;

			m_file = new QFile(send_file->getPath());
			m_file->open(QIODevice::ReadOnly);
			connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNextFilePiece(qint64)));
			emit fileTransferBeginning(send_file, this, m_socket->peerAddress().toString());
		}

		m_socket->write((char*)&response, sizeof(response));
	}
		break;
	case FILE_COMPLETED:
		m_socket->close();
		break;
	}
}


void ServerObject::sendNextListItem(qint64 bytes)
{
	(void)bytes;
	if(m_fileList->count() == m_items_sent)
	{
		m_socket->close();
		return;
	}
	qDebug() << "sending " << m_fileList->at(m_items_sent)->getName()
			 << ", ID: " << m_fileList->at(m_items_sent)->getId();

	m_socket->write(m_fileList->at(m_items_sent)->getByteArray());
	m_items_sent++;
}

void ServerObject::sendNextFilePiece(qint64 bytes)
{
	(void)bytes;
	if(m_file->atEnd())
	{
		m_file->close();
		delete m_file;	m_file = NULL;
		disconnect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNextFilePiece(qint64)));
		return;
	}

	emit progressUpdate(m_file->pos(), this);
	m_socket->write(m_file->read(FILE_CHUNK_SIZE));
}

void ServerObject::disconnected()
{
	if(m_socket) m_socket->deleteLater();
	if(m_file) m_file->close();

	emit finished();
}
