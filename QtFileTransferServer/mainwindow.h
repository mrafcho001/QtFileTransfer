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

namespace Ui {
class MainWindow;
}


//Forward declarations
class QProgressBar;
class QFrame;
class QThread;

class ProgressBarBundleServer
{
public:
	ProgressBarBundleServer();
	ProgressBarBundleServer(FileInfo* file, QString &ip, ServerObject *serverObj, QWidget *parent);
	~ProgressBarBundleServer();

	void insertIntoLayout(int reverse_index, QVBoxLayout *layout);
	void removeFromLayout(QVBoxLayout *layout);
	void update(qint64 value, double speed);

	void setCompleted();
	void setAborted();

private:
	QProgressBar *bar;
	QLabel *label;
	QFrame *hLine;

	FileInfo *file;
	ServerObject *server;
};

class ServerThreadBundle
{
public:
	ServerThreadBundle() : thread(NULL), servObj(NULL), progressBar(NULL)
	{
	}
	~ServerThreadBundle()
	{
		if(thread) delete thread;
		if(servObj) delete servObj;
		if(progressBar) delete progressBar;
	}

	QThread *thread;
	ServerObject *servObj;
	ProgressBarBundleServer *progressBar;
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
    Ui::MainWindow *ui;
	MyTcpServer *server;

	DirTreeModel *model;

	QList<FileInfo*> *m_serializedList;

	QSettings *settings;

	QHash<ServerObject*,ServerThreadBundle*> workerHash;
	QQueue<ServerThreadBundle*> toRemove;

};

#endif // MAINWINDOW_H
