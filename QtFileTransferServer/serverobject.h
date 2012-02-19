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
	
signals:
	void error(QTcpSocket::SocketError socketError);
	void finished();

	void fileTransferBeginning(FileInfo *file, ServerObject *obj, QString ip);
	void fileTransferUpdated(qint64 bytes_sent, double speed, ServerObject *obj);
	void fileTransferCompleted(ServerObject *obj);
	void fileTransferAborted(ServerObject *obj);

	void fileListRequested();
	void fileListTransferCompleted();

public slots:
	void handleConnection();
	void cleanupRequest();

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

	QTimer *m_uiTimer;

	QTime *m_speedTimer;
	int m_runningByteTotal;
	int m_runningTimeTotal;
	int m_byteHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_timeHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_headIndex;


	
};

#endif // SERVERTHREAD_H
