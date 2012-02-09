#ifndef FILELIST_H
#define FILELIST_H

#include <QAbstractItemModel>
#include <QList>
#include "../fileinfo.h"

class FileListItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
	FileListItemModel(QObject *parent = 0);
	//FileListItemModel(QList<FileInfo> &list, QObject *parent = 0);
	~FileListItemModel();

	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &index) const;

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;

	bool appendRowWithData(FileInfo & fileInfo, QModelIndex &parent);
	QList<FileInfo*>* getList();

private:

	FileInfo *getItem(const QModelIndex &index) const;
	FileInfo *rootItem;
    
};

#endif // FILELIST_H
