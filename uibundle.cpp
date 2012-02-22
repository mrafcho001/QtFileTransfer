#include "uibundle.h"
#include <QGridLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include "../sharedstructures.h"


UIBundle::UIBundle() :
	layout(NULL), pbProgress(NULL), lblFilName(NULL), lblSpeed(NULL), hLine(NULL),
	pbAction(NULL)
{
}

UIBundle::UIBundle(QWidget *parent)
{
	layout = new QGridLayout(0);


	lblFilName = new QLabel(parent);//file->getName(), parent);
	layout->addWidget(lblFilName, 0, 0, 1, 3);

	pbProgress = new QProgressBar(parent);
	pbProgress->setMaximum(100);//file->getSize());
	pbProgress->setValue(0);
	pbProgress->setTextVisible(true);
	layout->addWidget(pbProgress, 1, 0, 1, 3);

	pbAction = new QToolButton(parent);
	pbAction->setIcon(QIcon(QString(":/icons/stop.png")));
	layout->addWidget(pbAction, 1, 3);
	//Should be connected by subclasses
	//pbAction->connect(pbAction, SIGNAL(clicked()), clientObj, SLOT(abortFileTransfer()));

	lblSpeed = new QLabel("Speed: 0 Kb/s");
	layout->addWidget(lblSpeed, 2, 0);

	lblTimeRemaining = new QLabel("Time Remaining: 00:00:00", parent);
	layout->addWidget(lblTimeRemaining, 2, 1);

	lblTimeDownloading = new QLabel("Time Downloading: 00:00:00", parent);
	layout->addWidget(lblTimeDownloading, 2, 2);

	hLine = new QFrame(parent);
	hLine->setFrameShape(QFrame::HLine);
	layout->addWidget(hLine, 3, 0, 1, 3);
}

UIBundle::~UIBundle()
{
	clearLayout(layout);
	delete layout;
}

void UIBundle::insertIntoLayout(int reverse_index, QVBoxLayout *parentLayout)
{
	parentLayout->insertLayout(parentLayout->count()-reverse_index, this->layout);
}

void UIBundle::removeFromLayout(QVBoxLayout *parentLayout)
{
	parentLayout->removeItem(this->layout);
}

void UIBundle::update(const qint64 value, const double speed, const int ms_downloading, const int ms_remaining)
{
	pbProgress->setValue(value);
	lblSpeed->setText(QString("Speed: %1 Kb/s").arg(speed, 0, 'f', 2));

	lblTimeDownloading->setText(QString("Time Downloading: %1:%2:%3").arg(MS_TO_H(ms_downloading)).arg(MS_TO_M(ms_downloading)).arg(MS_TO_S(ms_downloading)));

	lblTimeRemaining->setText(QString("Time Remaining: %1:%2:%3").arg(MS_TO_H(ms_remaining)).arg(MS_TO_M(ms_remaining)).arg(MS_TO_S(ms_remaining)));
}

void UIBundle::setFinished()
{
	layout->removeWidget(pbAction);
	delete pbAction;
	pbProgress->setValue(pbProgress->maximum());
	lblSpeed->setText(lblSpeed->text().replace("Speed", "Avg Speed"));
}

void UIBundle::setAborted()
{
	pbAction->setIcon(QIcon(":/icons/restart.png"));
	pbAction->disconnect();
	//Do in subclases
	//pbAction->connect(pbAction, SIGNAL(clicked()), client, SLOT(resumeFileTransfer()));
	lblSpeed->setText(QString("Download Aborted"));
}

void UIBundle::setResumed()
{
	pbAction->setIcon(QIcon(":/icons/stop.png"));
	pbAction->disconnect();
	//Do in subclasses
	//pbAction->connect(pbAction, SIGNAL(clicked()), client, SLOT(abortFileTransfer()));
	lblSpeed->setText(QString("Download Resuming..."));
}

QToolButton* UIBundle::getActionButton()
{
	return pbAction;
}

void UIBundle::clearLayout(QLayout *layout)
{
	QLayoutItem *item;
	while((item = layout->takeAt(0)))
	{
		if(item->layout())
		{
			clearLayout(item->layout());
			delete item->layout();
		}
		else if(item->widget())
		{
			delete item->widget();
		}
		else if(item->spacerItem())
		{
			delete item->spacerItem();
		}
	}
}
