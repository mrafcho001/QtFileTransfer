#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>
#include <QHostAddress>
#include <QTime>

ServerObject::ServerObject(int sockDescriptor, QList<FileInfo*> *file_list, QObject *parent) :
	QObject(parent), m_currentMode(NONE), m_socketDescriptor(sockDescriptor), m_socket(NULL),
	m_items_sent(0), m_file(NULL), m_fileInfo(NULL), m_totalBytesSent(0), m_sessionTransfered(0),
	m_uiTimer(NULL), m_uiUpdateInterval(500), m_speedTimer(NULL), m_runningByteTotal(0),
	m_runningTimeTotal(0), m_headIndex(0)
{
	m_fileList = file_list;

	for(int i = 0; i < DOWNLOADRATE_HISTORY_SIZE; i++)
	{
		m_byteHistory[i] = 0;
		m_timeHistory[i] = 0;
	}
}
ServerObject::~ServerObject()
{
	if(m_socket)
	{
		m_socket->disconnect();
		m_socket->close();
		m_socket->deleteLater();
	}

	if(m_file)
	{
		m_file->close();
		delete m_file;
	}

	if(m_speedTimer) delete m_speedTimer;
	if(m_uiTimer) delete m_uiTimer;
}

double ServerObject::getCurrentSpeed()
{
	return getSpeed();
}

int ServerObject::getTimeDownloading()
{
	if(!m_avgTimer)
		return 0;

	return m_avgTimer->elapsed();
}

int ServerObject::getTimeRemaining()
{
	return (int)(((m_fileInfo->getSize()-m_totalBytesSent)/1024.0)/getSpeed()*1000);
}

bool ServerObject::setUpdateInterval(int ms)
{
	if(ms < 100)
		return false;

	m_uiUpdateInterval = ms;

	if(m_uiTimer)
	{
		if(m_uiTimer->isActive())
		{
			m_uiTimer->setInterval(ms);
		}
	}
	return true;
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


void ServerObject::cleanupRequest()
{
	if(m_socket)
	{
		m_socket->disconnect();
		m_socket->close();
	}
	if(m_uiTimer)
	{
		m_uiTimer->disconnect();
		m_uiTimer->stop();
	}
	if(m_file)
	{
		m_file->close();
	}

	this->disconnect();
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
		m_uiTimer->disconnect();
		m_uiTimer->stop();
		emit fileTransferUpdated(m_fileInfo->getSize(), 0, this);
		emit fileTransferCompleted(this);

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
	updateSpeed(bytes, m_speedTimer->elapsed());
	m_speedTimer->restart();

	m_totalBytesSent += bytes;

	if(m_socket->bytesToWrite() > FILE_CHUNK_SIZE
			|| m_file->atEnd())
		return;

	m_socket->write(m_file->read(FILE_CHUNK_SIZE));
}

void ServerObject::disconnected()
{
	m_socket->disconnect();
	if(m_file)
	{
		if(m_file->pos() < m_fileInfo->getSize())
			emit fileTransferAborted(this);

		m_file->close();
	}

	if(m_uiTimer)
		m_uiTimer->stop();

	emit finished();
}


void ServerObject::fileTransferSocketError(QAbstractSocket::SocketError err)
{
	if(err == QAbstractSocket::RemoteHostClosedError
			|| err == QAbstractSocket::NetworkError
			|| err == QAbstractSocket::UnknownSocketError)
	{
		m_socket->close();
		qDebug() << "Handled Error";
	}
	else
	{
		qDebug() << "Unhandled Error";
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
			m_fileInfo = m_fileList->value(i);
			break;
		}
	}

	if(m_fileInfo != NULL)
	{
		m_file = new QFile(m_fileInfo->getPath());

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

				emit fileTransferBeginning(m_fileInfo, this, m_socket->peerAddress().toString());
				m_speedTimer = new QTime();
				m_speedTimer->restart();
				m_uiTimer = new QTimer(0);
				connect(m_uiTimer, SIGNAL(timeout()), this, SLOT(triggerUIupdate()));
				m_uiTimer->start(m_uiUpdateInterval);
			}
		}
	}

	m_socket->write((char*)&response, sizeof(response));
	m_socket->waitForBytesWritten();
	if(response.message == FILE_DOWNLOAD_REQUEST_REJECTED
			|| response.message == PARTIAL_FILE_REQUEST_REJECTED)
	{
		m_socket->close();
	}

}

void ServerObject::updateSpeed(int bytes_sent, int ms)
{
	m_headIndex = (m_headIndex+1)%DOWNLOADRATE_HISTORY_SIZE;

	//Add new bytes to total and remove the tail bytes
	m_runningByteTotal += bytes_sent - m_byteHistory[m_headIndex];
	m_runningTimeTotal += ms - m_timeHistory[m_headIndex];

	//update the head bytes
	m_byteHistory[m_headIndex] = bytes_sent;
	m_timeHistory[m_headIndex] = ms;
}

double ServerObject::getSpeed()
{
	return (m_runningByteTotal/1024.0)/(m_runningTimeTotal/1000.0);
}


void ServerObject::triggerUIupdate()
{
	emit fileTransferUpdated(m_file->pos(), getSpeed(), this);
}
