#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include "mytcpserver.h"
#include "../fileinfo.h"
#include <QList>
#include <QSettings>
#include "dirtreemodel.h"
#include "serverobject.h"
#include <QQueue>
#include <QVBoxLayout>
#include <QLabel>
#include <QThread>
#include "../uibundle.h"

namespace Ui {
class MainWindow;
}


//Forward declarations
class QThread;

class ServerUIBundle : public UIBundle
{
public:
	ServerUIBundle();
	ServerUIBundle(FileInfo* file, QString &ip, ServerObject *serverObj, QWidget *parent);
	~ServerUIBundle();

	void update(qint64 value, double speed);

	void setAborted();

private:

	FileInfo *file;
	ServerObject *server;
};

class ServerWorkerBundle
{
public:
	ServerWorkerBundle() : thread(NULL), servObj(NULL), ui(NULL)
	{
	}
	~ServerWorkerBundle()
	{
		if(ui) delete ui;
		if(thread)
		{
			thread->quit();
			thread->wait();
		}
		if(servObj) delete servObj;
		if(thread)delete thread;
	}

	QThread *thread;
	ServerObject *servObj;
	ServerUIBundle *ui;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void closeEvent(QCloseEvent *event);

signals:

	void stopAllThreads();

public slots:
	void newConnection(int socketDescriptor);

	void removeSelected();
	void addNewDirectory();

	void fileTransferInitiated(FileInfo *file, ServerObject *obj, QString peer_ip);
	void fileTransferUpdate(qint64 bytes, double speed, ServerObject *obj);
	void fileTransferCompleted(ServerObject *obj);
	void fileTransferAborted(ServerObject *obj);
	void removeFileTransferUI();
    
private:
	void setForRemoval(ServerObject *obj);

    Ui::MainWindow *ui;
	MyTcpServer *server;

	DirTreeModel *model;

	QList<FileInfo*> *m_serializedList;

	QSettings *settings;

	QHash<ServerObject*,ServerWorkerBundle*> workerHash;
	QQueue<ServerWorkerBundle*> toRemove;

};

#endif // MAINWINDOW_H
