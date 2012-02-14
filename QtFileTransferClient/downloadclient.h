#ifndef DOWNLOADCLIENT_H
#define DOWNLOADCLIENT_H

#include <QObject>
#include <QHostAddress>
#include "../fileinfo.h"

//Forward declarations
class QFile;
class QTcpSocket;

class DownloadClient : public QObject
{
	Q_OBJECT
public:
	DownloadClient(QObject *parent = 0);
	DownloadClient(FileInfo *fileInfo, QObject *parent = 0);


	bool setRequestFile(FileInfo* file);
	bool setServerAddress(QHostAddress addr, quint16 port);
	bool setSaveDirectory(QString &dir);
	
signals:

	void fileTransferBeginning(FileInfo* file, DownloadClient* dc);
	void fileTransferUpdate(qint64 bytes_recieved, DownloadClient *dc);
	void fileTransferComplete(DownloadClient *dc);

	void finished();
	
public slots:

	void beginDownload();

private slots:
	void responseHandle();
	void dataReceive();

	void connectedHandle();
	void disconnectedHandle();
	void errorHandle(QAbstractSocket::SocketError err);

private:
	bool initFileForWriting();
	bool completeAndClose();


	enum opMode { IDLE, DOWNLOADING, REQUEST_PENDING };

	opMode m_currentMode;

	QTcpSocket *m_socket;
	QHostAddress m_serverAddress;
	quint16 m_serverPort;

	QString m_saveDirectory;
	QFile *m_outFile;
	FileInfo *m_fileInfo;

	qint64 m_bytesDownloaded;

	
};

#endif // DOWNLOADCLIENT_H
