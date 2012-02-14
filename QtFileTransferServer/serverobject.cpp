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
		emit finished();
		return;
	}

	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readReady()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
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

	switch(msg.message)
	{
	case REQUEST_FILE_LIST:

		listRequest();

		break;
	case REQUEST_FILE_DOWNLOAD:
	case REQUEST_PARTIAL_FILE:

		fileRequest(msg);

		break;
	case FILE_COMPLETED:

	default:
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

	m_socket->write(m_fileList->at(m_items_sent)->getByteArray());
	m_items_sent++;
}

void ServerObject::sendNextFilePiece(qint64 bytes)
{
	(void)bytes;
	if(m_file->atEnd())
	{
		emit progressUpdate(m_file->pos(), this);
		m_file->close();
		delete m_file;	m_file = NULL;
		emit fileTransferCompleted(this);
		disconnect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNextFilePiece(qint64)));
		disconnect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
				   this, SLOT(fileTransferSocketError(QAbstractSocket::SocketError)));
		return;
	}

	emit progressUpdate(m_file->pos(), this);
	m_socket->write(m_file->read(FILE_CHUNK_SIZE));
}

void ServerObject::disconnected()
{
	if(m_socket) m_socket->deleteLater();
	if(m_file)
	{
		m_file->close();
		delete m_file;
	}

	emit finished();
}


void ServerObject::fileTransferSocketError(QAbstractSocket::SocketError err)
{
	if(err == QAbstractSocket::RemoteHostClosedError
			|| err == QAbstractSocket::NetworkError
			|| err == QAbstractSocket::UnknownSocketError)
	{
		m_socket->close();
	}
}


void ServerObject::listRequest()
{
	struct connControlMsg response;
	m_currentMode = SENDING_LIST;
	m_items_sent = 0;
	if(m_fileList != NULL && m_fileList->count() > 0)
		response.message = LIST_REQUEST_GRANTED;
	else
		response.message = LIST_REQUEST_EMPTY;

	m_socket->write((char*)&response, sizeof(response));
	connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNextListItem(qint64)));
}

void ServerObject::fileRequest(connControlMsg msg)
{
	struct connControlMsg response;
	response.message = static_cast<ConnectionControl>(msg.message<<2);
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

	if(send_file != NULL)
	{
		m_file = new QFile(send_file->getPath());

		if(m_file->open(QIODevice::ReadOnly))
		{
			if(msg.message == REQUEST_FILE_DOWNLOAD ||
				(msg.message == REQUEST_PARTIAL_FILE && m_file->seek(msg.pos)))
			{
				response.message = static_cast<ConnectionControl>(msg.message<<1);

				connect(m_socket, SIGNAL(bytesWritten(qint64)),
						this, SLOT(sendNextFilePiece(qint64)));

				connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
						this, SLOT(fileTransferSocketError(QAbstractSocket::SocketError)));

				emit fileTransferBeginning(send_file, this, m_socket->peerAddress().toString());
			}
		}
	}

	m_socket->write((char*)&response, sizeof(response));
	if(response.message == FILE_DOWNLOAD_REQUEST_REJECTED
			|| response.message == PARTIAL_FILE_REQUEST_REJECTED)
		m_socket->close();

}
