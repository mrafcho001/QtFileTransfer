#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>
#include <QThread>
#include <QFileDialog>
#include <QProgressBar>
#include <QTimer>
#include <QFrame>


ProgressBarBundleServer::ProgressBarBundleServer()
{
	bar = NULL;
	label = NULL;
	file = NULL;
	server = NULL;
}

ProgressBarBundleServer::ProgressBarBundleServer(FileInfo *file, QString &ip, ServerObject *serverObj, QWidget *parent)
{
	this->file = file;
	server = serverObj;
	label = new QLabel(file->getName().append(" -> ").append(ip).append(" @ "), parent);
	bar = new QProgressBar(parent);
	bar->setMaximum(file->getSize());
	bar->setValue(0);
	bar->setTextVisible(true);

	hLine = new QFrame(parent);
	hLine->setFrameShape(QFrame::HLine);
}

ProgressBarBundleServer::~ProgressBarBundleServer()
{
	if(bar) delete bar;
	if(label) delete label;
	if(hLine) delete hLine;
}

void ProgressBarBundleServer::insertIntoLayout(int reverse_index, QVBoxLayout *layout)
{
	layout->insertWidget(layout->count()-reverse_index, label);
	layout->insertWidget(layout->count()-reverse_index, bar);
	layout->insertWidget(layout->count()-reverse_index, hLine);
}


void ProgressBarBundleServer::removeFromLayout(QVBoxLayout *layout)
{
	layout->removeWidget(label);
	layout->removeWidget(bar);
	layout->removeWidget(hLine);

	delete bar; bar = NULL;
	delete label; label = NULL;
	delete hLine; hLine = NULL;
}

void ProgressBarBundleServer::update(qint64 value, double speed)
{
	bar->setValue(value);
	QString str = label->text();
	str.truncate(str.lastIndexOf(QChar('@'))+2);
	str.append(QString::number(speed));
	label->setText(str);
}
void ProgressBarBundleServer::setAborted()
{
	label->setText(file->getName().append(" Aborted!!"));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


	settings = new QSettings(tr("Martin Bakiev"), tr("QtFileTransfer"), this);

	model = new DirTreeModel(this);
	ui->tvDirList->setModel(model);
	ui->tvDirList->header()->setResizeMode(QHeaderView::ResizeToContents);

	int size = settings->beginReadArray("shared_directories");
	for(int i = 0; i < size; i++)
	{
		settings->setArrayIndex(i);
		model->addDirectory(settings->value("dir").toString());
	}
	settings->endArray();


	int portNumber = settings->value("server/portNumber", DEFAULT_SERVER_LISTEN_PORT).toInt();

	this->server = new MyTcpServer(portNumber, this);
	this->server->startServer();

	connect(this->server,SIGNAL(newConnectionDescriptor(int)), this, SLOT(newConnection(int)));
	connect(ui->pbRemoveDir, SIGNAL(clicked()), this, SLOT(removeSelected()));
	connect(ui->pbAddDir, SIGNAL(clicked()), this, SLOT(addNewDirectory()));

	//Keep serializedList up to date
	m_serializedList = new QList<FileInfo*>(model->getSerializedList());
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);

	//qDebug() << "Close event received";
	server->close();

	settings->beginWriteArray("shared_directories");
	QList<FileInfo*> sharedDirList = model->getSharedDirList();
	for(int i = 0; i < sharedDirList.count(); i++)
	{
		settings->setArrayIndex(i);
		settings->setValue("dir", sharedDirList.at(i)->getPath());
	}
	settings->endArray();


	delete server;
	delete settings;
	delete model;
	if(m_serializedList) delete m_serializedList;
}

void MainWindow::removeSelected()
{
	QModelIndex index = ui->tvDirList->currentIndex();

	//qDebug() << "Trying to remove";
	if(model->removeRows(0, 0, index))
		*m_serializedList = model->getSerializedList();
}

void MainWindow::addNewDirectory()
{
	QFileDialog dialog(this);

	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, false);

	if(settings->contains("server/last_added_dir"))
		dialog.setDirectory(settings->value("server/last_added_dir").toString());

	QStringList selectedDir;
	if(dialog.exec())
		selectedDir = dialog.selectedFiles();

	for(int i = 0; i < selectedDir.count(); i++)
	{
		model->addDirectory(selectedDir.at(0));
	}

	if(selectedDir.count()>0)
	{
		settings->setValue("server/last_added_dir", selectedDir.value(0));
		*m_serializedList = model->getSerializedList();
	}
}

void MainWindow::fileTransferInitiated(FileInfo *file, ServerObject *obj, QString peer_ip)
{
	ProgressBarBundleServer *pbb = new ProgressBarBundleServer(file, peer_ip, obj, this);

	pbb->insertIntoLayout(1, ui->vlProgressBarLayout);

	progressBars.insert(obj, pbb);
}

void MainWindow::fileTransferUpdate(qint64 bytes, double speed, ServerObject *obj)
{
	if(!progressBars.contains(obj))
		return;

	progressBars.value(obj)->update(bytes,speed);
}

void MainWindow::fileTransferCompleted(ServerObject *obj)
{
	toRemove.enqueue(obj);
	QTimer::singleShot(10000, this, SLOT(removePB()));
}

void MainWindow::fileTransferAborted(ServerObject *obj)
{
	progressBars.value(obj)->setAborted();
	fileTransferCompleted(obj);
}

void MainWindow::removePB()
{
	ServerObject *obj = toRemove.dequeue();
	progressBars.value(obj)->removeFromLayout(ui->vlProgressBarLayout);
	delete progressBars.value(obj);
	progressBars.remove(obj);
}

void MainWindow::newConnection(int socketDescriptor)
{
	//Handle New Connection

	//qDebug() << "New Connection: " << socketDescriptor;
	QThread *thread = new QThread(this);

	ServerObject *serverObject = new ServerObject(socketDescriptor, m_serializedList);

	serverObject->moveToThread(thread);
	connect(thread, SIGNAL(started()), serverObject, SLOT(handleConnection()));
	connect(serverObject, SIGNAL(finished()), thread, SLOT(quit()));
	connect(serverObject, SIGNAL(fileTransferBeginning(FileInfo*,ServerObject*,QString)),
			this, SLOT(fileTransferInitiated(FileInfo*,ServerObject*,QString)));

	connect(serverObject, SIGNAL(fileTransferCompleted(ServerObject*)),
			this, SLOT(fileTransferCompleted(ServerObject*)));

	connect(serverObject, SIGNAL(fileTransferAborted(ServerObject*)),
			this, SLOT(fileTransferAborted(ServerObject*)));

	connect(serverObject,  SIGNAL(progressUpdate(qint64,double,ServerObject*)),
			this, SLOT(fileTransferUpdate(qint64,double,ServerObject*)));



	thread->start();
}
