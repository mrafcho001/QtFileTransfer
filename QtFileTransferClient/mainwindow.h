#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filelistitemmodel.h"

#include <QTcpSocket>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
	void downloadFileList();
	void onListReceiveData();
	void onFileReceiveData();
	void requestFileDownload();

	void sock_disconn();
    
private:
    Ui::MainWindow *ui;
	FileListItemModel *tableModel;


	QTcpSocket *sock;

	QFile *out_file;
	FileInfo *rec_file;
	qint64 rec_bytes;
};

#endif // MAINWINDOW_H
