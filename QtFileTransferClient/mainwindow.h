#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filelistitemmodel.h"

#include <QTcpSocket>

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
		void testSlot();
		void reply();
    
private:
    Ui::MainWindow *ui;
	FileListItemModel *tableModel;


	QTcpSocket *sock;
};

#endif // MAINWINDOW_H
