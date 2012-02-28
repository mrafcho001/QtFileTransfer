#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QFile>
#include "../fileinfo.h"
#include "../sharedstructures.h"
#include <QTimer>

#define HISTORY_SIZE	10

class ServerObject : public QObject
{
	Q_OBJECT
public:
	ServerObject(int socketDescriptor, QList<FileInfo*> *file_list, QObject *parent = 0);
	~ServerObject();


	double getCurrentSpeed();
	int getTimeDownloading(); // in ms
	int getTimeRemaining();	// in ms
	bool setUpdateInterval(int ms);
	
signals:
	void error(QTcpSocket::SocketError socketError);
	void finished();

	void fileTransferBeginning(FileInfo *file, ServerObject *so, QString ip);
	void fileTransferUpdated(qint64 bytes_sent, double speed, ServerObject *so);
	void fileTransferCompleted(ServerObject *so);
	void fileTransferAborted(ServerObject *so);
	void fileTransferResumed(ServerObject *so);

	void fileListRequested();
	void fileListTransferCompleted();

public slots:
	void handleConnection();
	void cleanupRequest();
	void abortFileTransfer();

private slots:
	void readReady();
	void sendNextListItem(qint64 bytes);
	void sendNextFilePiece(qint64 bytes);

	void disconnected();
	void fileTransferSocketError(QAbstractSocket::SocketError err);

	void triggerUIupdate();

private:

	void listRequest();
	void fileRequest(connControlMsg msg);
	void updateSpeed(int bytes_sent, int ms);
	double getSpeed();


	enum opMode { NONE, SENDING_FILE, SENDING_LIST };


	opMode m_currentMode;
	int m_socketDescriptor;
	QTcpSocket *m_socket;

	QList<FileInfo*> *m_fileList;
	int m_items_sent;


	QFile *m_file;
	FileInfo *m_fileInfo;
	qint64 m_totalBytesSent;
	qint64 m_sessionTransfered;

	QTimer *m_uiTimer;
	unsigned int m_uiUpdateInterval;

	QTime *m_speedTimer;
	QTime *m_avgTimer;
	int m_runningByteTotal;
	int m_runningTimeTotal;
	int m_byteHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_timeHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_headIndex;


	
};

#endif // SERVERTHREAD_H
