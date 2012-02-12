#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filelistitemmodel.h"
#include "downloadclient.h"
#include <QTcpSocket>
#include <QFile>
#include <QQueue>
#include <QHash>

namespace Ui {
class MainWindow;
}

//Forward class declarations
class QProgressBar;
class QLabel;
class QVBoxLayout;
class QSettings;

class ProgressBarBundleClient
{
public:
	ProgressBarBundleClient();
	ProgressBarBundleClient(FileInfo* file, DownloadClient *clientObj, QWidget *parent = 0);
	~ProgressBarBundleClient();

	void insertIntoLayout(int reverse_index, QVBoxLayout *layout);
	void removeFromLayout(QVBoxLayout *layout);
	void update(qint64 value);

private:
	QProgressBar *bar;
	QLabel *label;
	FileInfo *file;
	DownloadClient *client;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
	void downloadFileList();
	void requestFileDownload();
	void selectNewSaveDirectory();

	void fileTransferStarted(FileInfo* file, DownloadClient* dc);
	void fileTransferUpdated(qint64 bytes, DownloadClient *dc);
	void fileTransferCompleted(DownloadClient *dc);

	void removeProgresBar();


	void sock_disconn();
	void onListReceiveData();

private:
	bool getServerAddress(QHostAddress *addr);

    Ui::MainWindow *ui;
	FileListItemModel *tableModel;

	QSettings *settings;

	QTcpSocket *m_socket;

	QFile *out_file;
	FileInfo *rec_file;
	qint64 rec_bytes;

	bool list_ack_receieved;

	QHash<DownloadClient*,ProgressBarBundleClient*> activeDownloads;
	QQueue<DownloadClient*> toRemove;
};

#endif // MAINWINDOW_H
