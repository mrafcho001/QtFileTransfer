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

namespace Ui {
class MainWindow;
}


//Forward declarations
class QProgressBar;
class QFrame;

class ProgressBarBundleServer
{
public:
	ProgressBarBundleServer();
	ProgressBarBundleServer(FileInfo* file, QString &ip, ServerObject *serverObj, QWidget *parent);
	~ProgressBarBundleServer();

	void insertIntoLayout(int reverse_index, QVBoxLayout *layout);
	void removeFromLayout(QVBoxLayout *layout);
	void update(qint64 value, double speed);

	void setAborted();

private:
	QProgressBar *bar;
	QLabel *label;
	QFrame *hLine;
	FileInfo *file;
	ServerObject *server;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void closeEvent(QCloseEvent *event);

public slots:
	void newConnection(int socketDescriptor);

	void removeSelected();
	void addNewDirectory();

	void fileTransferInitiated(FileInfo *file, ServerObject *obj, QString peer_ip);
	void fileTransferUpdate(qint64 bytes, double speed, ServerObject *obj);
	void fileTransferCompleted(ServerObject *obj);
	void fileTransferAborted(ServerObject *obj);
	void removePB();
    
private:
    Ui::MainWindow *ui;
	MyTcpServer *server;

	DirTreeModel *model;

	QList<FileInfo*> *m_serializedList;

	QSettings *settings;

	QHash<ServerObject*,ProgressBarBundleServer*> progressBars;
	QQueue<ServerObject*> toRemove;

};

#endif // MAINWINDOW_H
