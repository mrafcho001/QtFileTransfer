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
#include <QCloseEvent>

ServerUIBundle::ServerUIBundle()
{
	bar = NULL;
	label = NULL;
	file = NULL;
	server = NULL;
}

ServerUIBundle::ServerUIBundle(FileInfo *file, QString &ip, ServerObject *serverObj, QWidget *parent)
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

ServerUIBundle::~ServerUIBundle()
{
	if(bar) delete bar;
	if(label) delete label;
	if(hLine) delete hLine;
}

void ServerUIBundle::insertIntoLayout(int reverse_index, QVBoxLayout *layout)
{
	layout->insertWidget(layout->count()-reverse_index, label);
	layout->insertWidget(layout->count()-reverse_index, bar);
	layout->insertWidget(layout->count()-reverse_index, hLine);
}


void ServerUIBundle::removeFromLayout(QVBoxLayout *layout)
{
	layout->removeWidget(label);
	layout->removeWidget(bar);
	layout->removeWidget(hLine);
}

void ServerUIBundle::update(qint64 value, double speed)
{
	bar->setValue(value);
	QString str = label->text();
	str.truncate(str.lastIndexOf(QChar('@'))+2);
	str.append(QString::number(speed));
	label->setText(str);
}

void ServerUIBundle::setCompleted()
{
	label->setText(file->getName().append(" Completed!"));
}

void ServerUIBundle::setAborted()
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
	emit stopAllThreads();

	server->close();

	settings->beginWriteArray("shared_directories");
	QList<FileInfo*> sharedDirList = model->getSharedDirList();
	for(int i = 0; i < sharedDirList.count(); i++)
	{
		settings->setArrayIndex(i);
		settings->setValue("dir", sharedDirList.at(i)->getPath());
	}
	settings->endArray();

	QHashIterator<ServerObject*,ServerWorkerBundle*> iter(workerHash);
	while(iter.hasNext())
	{
		iter.next();
		iter.value()->thread->quit();
		iter.value()->thread->wait();
		delete iter.value();
	}

	delete server;
	delete settings;
	delete model;
	if(m_serializedList) delete m_serializedList;
	event->setAccepted(true);
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
	if(!workerHash.contains(obj))
		return;

	ServerWorkerBundle *worker = workerHash.value(obj);

	worker->progressBar = new ServerUIBundle(file, peer_ip, obj, this);
	worker->progressBar->insertIntoLayout(1, ui->vlProgressBarLayout);
}

void MainWindow::fileTransferUpdate(qint64 bytes, double speed, ServerObject *obj)
{
	if(!workerHash.contains(obj))
		return;

	workerHash.value(obj)->progressBar->update(bytes,speed);
}

void MainWindow::fileTransferCompleted(ServerObject *obj)
{
	toRemove.enqueue(workerHash.value(obj));
	workerHash.remove(obj);

	QTimer::singleShot(10000, this, SLOT(removeFileTransferUI()));
}

void MainWindow::fileTransferAborted(ServerObject *obj)
{
	if(!workerHash.contains(obj))
		return;

	workerHash.value(obj)->progressBar->setAborted();
	fileTransferCompleted(obj);
}

void MainWindow::removeFileTransferUI()
{
	ServerWorkerBundle *worker = toRemove.dequeue();
	worker->progressBar->removeFromLayout(ui->vlProgressBarLayout);
	worker->thread->disconnect();

	delete worker;
}

void MainWindow::newConnection(int socketDescriptor)
{
	//Handle New Connection

	ServerWorkerBundle *worker = new ServerWorkerBundle();

	worker->thread = new QThread(this);
	worker->servObj = new ServerObject(socketDescriptor, m_serializedList);

	workerHash.insert(worker->servObj, worker);

	worker->servObj->moveToThread(worker->thread);


	connect(worker->thread, SIGNAL(started()), worker->servObj, SLOT(handleConnection()));
	connect(worker->servObj, SIGNAL(finished()), worker->thread, SLOT(quit()));

	connect(worker->servObj, SIGNAL(fileTransferBeginning(FileInfo*,ServerObject*,QString)),
			this, SLOT(fileTransferInitiated(FileInfo*,ServerObject*,QString)));

	connect(worker->servObj, SIGNAL(fileTransferCompleted(ServerObject*)),
			this, SLOT(fileTransferCompleted(ServerObject*)));

	connect(worker->servObj, SIGNAL(fileTransferAborted(ServerObject*)),
			this, SLOT(fileTransferAborted(ServerObject*)));

	connect(worker->servObj,  SIGNAL(fileTransferUpdated(qint64,double,ServerObject*)),
			this, SLOT(fileTransferUpdate(qint64,double,ServerObject*)));

	connect(this, SIGNAL(stopAllThreads()), worker->servObj, SLOT(cleanupRequest()));

	worker->thread->start();
}
