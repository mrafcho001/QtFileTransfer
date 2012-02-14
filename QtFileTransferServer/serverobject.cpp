#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>
#include <QHostAddress>
#include <QTime>

ServerObject::ServerObject(int sockDescriptor, QList<FileInfo*> *file_list, QObject *parent) :
	QObject(parent), m_socketDescriptor(sockDescriptor), m_socket(NULL), m_file(NULL),
	timer(NULL), head_index(0), regular_ui_updates(NULL)
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


void ServerObject::quitRequest()
{
	qDebug() << "Should trigger Disconnected()";
	m_socket->close();
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
	update_speed(bytes, timer->elapsed());
	timer->restart();

	bytes_sent_total += bytes;


	emit progressUpdate(m_file->pos(), getSpeed(), this);

	if(m_socket->bytesToWrite() > 2*FILE_CHUNK_SIZE
			|| m_file->atEnd())
		return;

	m_socket->write(m_file->read(FILE_CHUNK_SIZE));
}

void ServerObject::disconnected()
{
	qDebug() << "Disconnected triggered";
	m_socket->disconnect();
	m_socket->deleteLater();
	if(m_file)
	{
		if(m_file->pos() < send_file->getSize())
		{
			emit fileTransferAborted(this);
		}

		m_file->close();
		delete m_file;
	}

	if(timer) delete timer;
	if(regular_ui_updates) delete regular_ui_updates;

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
				timer = new QTime();
				timer->restart();
				regular_ui_updates = new QTimer(0);
				connect(regular_ui_updates, SIGNAL(timeout()), this, SLOT(triggerUIupdate()));
				regular_ui_updates->start(750);
			}
		}
	}

	m_socket->write((char*)&response, sizeof(response));
	if(response.message == FILE_DOWNLOAD_REQUEST_REJECTED
			|| response.message == PARTIAL_FILE_REQUEST_REJECTED)
	{
		m_socket->close();
	}

}

void ServerObject::update_speed(int bytes_sent, int ms)
{
	head_index = (head_index+1)%HISTORY_SIZE;

	//Add new bytes to total and remove the tail bytes
	running_byte_total += bytes_sent - prev_bytes_sent[head_index];
	running_ms_total += ms - prev_times_sent[head_index];

	//update the head bytes
	prev_bytes_sent[head_index] = bytes_sent;
	prev_times_sent[head_index] = ms;
}

double ServerObject::getSpeed()
{
	return (running_byte_total/1024.0)/(running_ms_total/1000.0);
}


void ServerObject::triggerUIupdate()
{
	emit progressUpdate(m_file->pos(), getSpeed(), this);
}
