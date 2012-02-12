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
	ServerObject(int socketDescriptor, QList<FileInfo*> *file_list, QObject *parent = 0);

	
signals:
	void error(QTcpSocket::SocketError socketError);
	void finished();

	void progressUpdate(qint64 bytes_sent, ServerObject *obj);
	void fileTransferBeginning(FileInfo *file, ServerObject *obj, QString ip);
	void fileListRequested();
	void fileTransferCompleted(ServerObject *obj);
	void fileListTransferCompleted();

public slots:
	void handleConnection();

	void sendNextListItem(qint64 bytes);
	void sendNextFilePiece(qint64 bytes);

	void readReady();

	void disconnected();

private:
	enum opMode { NONE, SENDING_FILE, SENDING_LIST };


	opMode m_currentMode;
	int m_socketDescriptor;
	QTcpSocket *m_socket;

	QList<FileInfo*> *m_fileList;
	int m_items_sent;


	QFile *m_file;
	FileInfo *send_file;
	
};

#endif // SERVERTHREAD_H
