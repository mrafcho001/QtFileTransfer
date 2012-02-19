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
	
signals:

	void fileTransferBeginning(FileInfo* file, DownloadClient* dc);
	void fileTransferUpdate(qint64 bytes_recieved, double speed, DownloadClient *dc);
	void fileTransferComplete(DownloadClient *dc);
	void fileTransferAborted(qint64 bytes_recieved, DownloadClient *dc);

	void finished();
	
public slots:

	void beginDownload();

private slots:
	void responseHandle();
	void dataReceive();

	void connectedHandle();
	void disconnectedHandle();
	void errorHandle(QAbstractSocket::SocketError err);

	void triggerUIupdate();

private:
	bool initFileForWriting();
	bool completeAndClose();

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

	qint64 m_bytesDownloaded;


	QTimer *m_uiTimer;

	QTime *m_speedTimer;
	int m_runningByteTotal;
	int m_runningTimeTotal;
	int m_byteHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_timeHistory[DOWNLOADRATE_HISTORY_SIZE];
	int m_headIndex;

	
};

#endif // DOWNLOADCLIENT_H
