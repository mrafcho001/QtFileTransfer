#include "downloadclient.h"
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QTime>
#include "../fileinfo.h"
#include "../sharedstructures.h"

DownloadClient::DownloadClient(QObject *parent) :
	QObject(parent), m_currentMode(SETUP), m_socket(NULL), m_outFile(NULL), m_fileInfo(NULL), m_bytesDownloaded(0)
{
}

DownloadClient::DownloadClient(FileInfo *fileInfo, QObject *parent) :
	QObject(parent),m_currentMode(SETUP), m_socket(NULL), m_outFile(NULL), m_bytesDownloaded(0)
{
	m_fileInfo = new FileInfo(*fileInfo);
}

DownloadClient::~DownloadClient()
{
	delete m_fileInfo;
}

bool DownloadClient::setRequestFile(FileInfo* file)
{
	if(m_currentMode == SETUP)
	{
		m_fileInfo = new FileInfo(*file);
		return true;
	}
	return false;
}

bool DownloadClient::setServerAddress(QHostAddress addr, quint16 port)
{
	if(m_currentMode == SETUP)
	{
		m_serverAddress = addr;
		m_serverPort = port;
		return true;
	}
	return false;
}

bool DownloadClient::setSaveDirectory(QString &dir)
{
	if(m_currentMode == SETUP)
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
	if(m_currentMode != SETUP)
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
		m_currentMode = REJECTED;
		qCritical() << "File Request denied.";
		m_socket->close();
		return;
	}

	disconnect(m_socket, SIGNAL(readyRead()), 0, 0);
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(dataReceive()));

	m_currentMode = DOWNLOADING;
	m_bytesDownloaded = 0;

	m_speedTimer = new QTime();

	m_uiTimer = new QTimer();
	connect(m_uiTimer, SIGNAL(timeout()), this, SLOT(triggerUIupdate()));
	m_uiTimer->start(750);

	emit fileTransferBeginning(m_fileInfo, this);
}

void DownloadClient::dataReceive()
{

	qint64 bytesWriten = m_outFile->write(m_socket->readAll());

	updateSpeed(bytesWriten, m_speedTimer->elapsed());
	m_speedTimer->restart();

	m_bytesDownloaded += bytesWriten;

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
	//connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnectedHandle()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorHandle(QAbstractSocket::SocketError)));

}

void DownloadClient::disconnectedHandle()
{
	if(m_uiTimer) delete m_uiTimer;
	if(m_speedTimer) delete m_speedTimer;
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
		m_socket->deleteLater();
		if(m_outFile)
		{
			if(m_outFile->pos() < m_fileInfo->getSize())
			{
				emit fileTransferAborted(m_outFile->pos(), this);
			}
			m_outFile->close();
			delete m_outFile;
		}
		emit finished();
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

void DownloadClient::updateSpeed(int bytes_sent, int ms)
{
	m_headIndex = (m_headIndex+1)%DOWNLOADRATE_HISTORY_SIZE;

	//Add new bytes to total and remove the tail bytes
	m_runningByteTotal += bytes_sent - m_byteHistory[m_headIndex];
	m_runningTimeTotal += ms - m_timeHistory[m_headIndex];

	//update the head bytes
	m_byteHistory[m_headIndex] = bytes_sent;
	m_timeHistory[m_headIndex] = ms;
}

double DownloadClient::getSpeed()
{
	return (m_runningByteTotal/1024.0)/(m_runningTimeTotal/1000.0);
}


void DownloadClient::triggerUIupdate()
{
	emit fileTransferUpdate(m_bytesDownloaded, getSpeed(), this);
}
