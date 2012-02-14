#include "downloadclient.h"
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include "../fileinfo.h"
#include "../sharedstructures.h"

DownloadClient::DownloadClient(QObject *parent) :
	QObject(parent), m_currentMode(IDLE), m_socket(NULL), m_outFile(NULL), m_fileInfo(NULL), m_bytesDownloaded(0)
{
}

DownloadClient::DownloadClient(FileInfo *fileInfo, QObject *parent) :
	QObject(parent),m_currentMode(IDLE), m_socket(NULL), m_outFile(NULL), m_bytesDownloaded(0)
{
	m_fileInfo = fileInfo;
}


bool DownloadClient::setRequestFile(FileInfo* file)
{
	if(m_currentMode == IDLE)
	{
		m_fileInfo = file;
		return true;
	}
	return false;
}

bool DownloadClient::setServerAddress(QHostAddress addr, quint16 port)
{
	if(m_currentMode == IDLE)
	{
		m_serverAddress = addr;
		m_serverPort = port;
		return true;
	}
	return false;
}

bool DownloadClient::setSaveDirectory(QString &dir)
{
	if(m_currentMode == IDLE)
	{
		m_saveDirectory = dir;
		return true;
	}
	return false;
}

void DownloadClient::beginDownload()
{
	if(m_fileInfo == NULL)
	{
		qCritical() << "No File selected for downloading. Action canceled.";
		return;
	}
	if(m_currentMode != IDLE)
	{
		qCritical() << "File transfer already in progress. Action canceled";
		return;
	}
	if(m_saveDirectory.isEmpty())
	{
		qCritical() << "No save directory set. Action canceled.";
		return;
	}

	if(!initFileForWriting())
	{
		qCritical() << "Could not create file for writing. Action canceled.";
		return;
	}

	m_socket = new QTcpSocket();

	connect(m_socket, SIGNAL(connected()), this, SLOT(connectedHandle()));
	m_socket->connectToHost(m_serverAddress, m_serverPort);
}

void DownloadClient::responseHandle()
{
	if(m_socket->bytesAvailable() < (qint64)sizeof(connControlMsg))
	{
		qCritical() << "Did not recieve proper response.  Quiting file download";
		m_socket->close();
		return;
	}

	connControlMsg response;
	m_socket->read((char*)&response, sizeof(response));

	if(response.message == FILE_DOWNLOAD_REQUEST_REJECTED)
	{
		qCritical() << "File Request denied.";
		m_socket->close();
		return;
	}

	disconnect(m_socket, SIGNAL(readyRead()), 0, 0);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(dataReceive()));

	m_currentMode = DOWNLOADING;
	m_bytesDownloaded = 0;
	emit fileTransferBeginning(m_fileInfo, this);

	if(m_socket->bytesAvailable() > 0)
		dataReceive();
}

void DownloadClient::dataReceive()
{
	m_bytesDownloaded += m_outFile->write(m_socket->readAll());
	emit fileTransferUpdate(m_bytesDownloaded, this);

	if(m_bytesDownloaded >= m_fileInfo->getSize())
		completeAndClose();
}

void DownloadClient::connectedHandle()
{
	if(!m_socket->isValid())
	{
		qCritical() << "Could not connect to server.  Action canceled.";
		return;
	}

	m_currentMode = REQUEST_PENDING;

	connControlMsg msg;
	msg.message = REQUEST_FILE_DOWNLOAD;
	memcpy(msg.sha1_id, m_fileInfo->getHash().constData(), SHA1_BYTECOUNT);

	m_socket->write((char*)&msg, sizeof(msg));

	connect(m_socket, SIGNAL(readyRead()), this, SLOT(responseHandle()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnectedHandle()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorHandle(QAbstractSocket::SocketError)));

}

void DownloadClient::disconnectedHandle()
{
	if(m_outFile) delete m_outFile;
	if(m_socket)  m_socket->deleteLater();
	emit finished();
}

void DownloadClient::errorHandle(QAbstractSocket::SocketError err)
{
	if(err == QAbstractSocket::RemoteHostClosedError
			|| err == QAbstractSocket::NetworkError
			|| err == QAbstractSocket::UnknownSocketError)
	{
		m_socket->close();
		m_outFile->close();
	}
}

bool DownloadClient::initFileForWriting()
{
	QDir dir(m_saveDirectory);
	if(!dir.exists())
		return false;

	m_outFile = new QFile(dir.absoluteFilePath(m_fileInfo->getName()));

	return m_outFile->open(QIODevice::ReadWrite);
}

bool DownloadClient::completeAndClose()
{
	m_outFile->close();
	emit fileTransferComplete(this);

	connControlMsg msg;
	msg.message = FILE_COMPLETED;
	memcpy(msg.sha1_id, m_fileInfo->getHash().constData(), SHA1_BYTECOUNT);

	m_socket->write((char*)&msg, sizeof(msg));
	m_socket->close();
	return true;
}
