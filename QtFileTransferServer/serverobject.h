#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QFile>
#include "../fileinfo.h"

class ServerObject : public QObject
{
	Q_OBJECT
public:
	explicit ServerObject(int socketDescriptor, QList<FileInfo> *file_list, QObject *parent = 0);
	
signals:
	void error(QTcpSocket::SocketError socketError);
	void finished();

	void progressUpdate(float percent);
	void fileTransferBeginning();
	void fileListRequested();
	void fileTransferCompleted();
	void fileListTransferCompleted();

public slots:
	void handleConnection();

	void bytesWritten(qint64 bytes);

	void readReady();

	void disconnected();

private:
	enum opMode { NONE, SENDING_FILE, SENDING_LIST };


	opMode m_currentMode;
	int m_socketDescriptor;
	QTcpSocket *m_socket;

	QList<FileInfo> *m_fileList;
	unsigned int m_items_sent;
	QFile *m_file;
	qint64 m_file_size;
	qint64 m_bytes_sent;
	
};

#endif // SERVERTHREAD_H
