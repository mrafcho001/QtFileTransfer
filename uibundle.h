#ifndef UIBUNDLE_H
#define UIBUNDLE_H
#include <QtGlobal>
//#include <QWidget>

class QProgressBar;
class QLabel;
class QVBoxLayout;
class QFrame;
class QGridLayout;
class QToolButton;
class QWidget;
class QLayout;

class UIBundle
{
public:
	UIBundle();
	UIBundle(QWidget *parent);
	~UIBundle();

	virtual void insertIntoLayout(int reverse_index, QVBoxLayout *parentLayout);
	virtual void removeFromLayout(QVBoxLayout *parentLayout);
	void update(const qint64 value, const double speed, const int ms_downloading, const int ms_remaining);

	virtual void setFinished();
	virtual void setAborted();
	virtual void setResumed();

	QToolButton *getActionButton();

protected:
	void clearLayout(QLayout *layout);

	QGridLayout *layout;

	QProgressBar *pbProgress;
	QLabel *lblFilName;
	QLabel *lblSpeed;
	QLabel *lblTimeDownloading;
	QLabel *lblTimeRemaining;
	QFrame *hLine;
	QToolButton *pbAction;
};

#endif
