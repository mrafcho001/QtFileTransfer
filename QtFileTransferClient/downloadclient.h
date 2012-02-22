#ifndef DOWNLOADCLIENT_H
#define DOWNLOADCLIENT_H

#include <QObject>
#include <QTimer>
#include <QHostAddress>
#include "../fileinfo.h"
#include "../sharedstructures.h"

//Forward declarations
class QFile;
class QTcpSocket;

class DownloadClient : public QObject
{
	Q_OBJECT
public:
	DownloadClient(QObject *parent = 0);
	DownloadClient(FileInfo *fileInfo, QObject *parent = 0);
	~DownloadClient();


	bool setRequestFile(FileInfo* file);
	bool setServerAddress(QHostAddress addr, quint16 port);
	bool setSaveDirectory(QString &dir);
	bool setUpdateInterval(int ms);

	double getCurrentSpeed();
	int getTimeDownloading(); // in ms
	int getTimeRemaining();	// in ms
	
signals:

	void fileTransferBeginning(FileInfo* file, DownloadClient* dc);
	void fileTransferUpdate(qint64 bytes_recieved, double speed, DownloadClient *dc);
	void fileTransferComplete(DownloadClient *dc);
	void fileTransferAborted(qint64 bytes_recieved, DownloadClient *dc);
	void fileTransferResumed(DownloadClient *dc);

	void finished();
	
public slots:

	void beginDownload();
	void abortFileTransfer();
	void resumeFileTransfer();

	void cleanupRequest();

private slots:
	void connectedHandle();
	void responseHandle();
	void dataReceive();

	void disconnectedHandle();
	void errorHandle(QAbstractSocket::SocketError err);

	void triggerUIupdate();

private:
	void connectSocket();
	bool initFileForWriting(qint64 pos = 0);
	bool checkInit();
	bool completeAndClose();

	bool abortedCleanUp();
	bool cleanAndExit();

	void updateSpeed(int bytes_sent, int ms);
	double getSpeed();


	enum opMode { SETUP, DOWNLOADING, REQUEST_PENDING, REJECTED, ABORTED };

	opMode m_currentMode;

	QTcpSocket *m_socket;
	QHostAddress m_serverAddress;
	quint16 m_serverPort;

	QString m_saveDirectory;
	QFile *m_outFile;
	FileInfo *m_fileInfo;

	qint64 m_bytePosition;
	qint64 m_sessionDownloaded;


	QTimer *m_uiTimer;
	unsigned int m_uiUpdateInterval;

	QTime *m_speedTimer;
	QTime *m_avgTimer;
	int m_runningByteTotal;
	int m_runningTimeTotal;
	int m_byteHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_timeHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_headIndex;

	int m_total_time;

	
};

#endif // DOWNLOADCLIENT_H
