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

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void closeEvent(QCloseEvent *event);

public slots:
	void newConnection(int socketDescriptor);
	void buttonTest();
    
private:
    Ui::MainWindow *ui;
	MyTcpServer *server;

	DirTreeModel *model;

	QSettings *settings;
};

#endif // MAINWINDOW_H
