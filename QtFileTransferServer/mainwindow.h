#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include "mytcpserver.h"
#include "../fileinfo.h"
#include <QList>

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
	void newConnection(int socketDescriptor);
    
private:
    Ui::MainWindow *ui;
	MyTcpServer *server;
	QList<FileInfo>  *m_file_list;
};

#endif // MAINWINDOW_H
