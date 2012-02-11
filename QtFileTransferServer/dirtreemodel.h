#ifndef DIRTREEMODEL_H
#define DIRTREEMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include "../fileinfo.h"
#include <QHash>

class DirTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit DirTreeModel(QObject *parent = 0);
	
	~DirTreeModel();

	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &index) const;

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	bool removeRows(int row, int count, const QModelIndex &parent);

	Qt::ItemFlags flags(const QModelIndex &index) const;

	bool addDirectory(const QString &directory);
	bool addDirectory(const QString &directory, FileInfo *parentItem, QModelIndex &parent);
	bool addDirectory(const QString &directory, FileInfo *parentItem, const QModelIndex &parent, bool addEmpty);

	QList<FileInfo*> getSharedDirList();

	QList<FileInfo*> getSerializedList(FileInfo* root = NULL);

private:

	void removeFromHashRecursive(FileInfo* item);

	FileInfo *getItem(const QModelIndex &index) const;
	FileInfo *rootItem;
	QHash<QByteArray, FileInfo*> hash;
	
};

#endif // DIRTREEMODEL_H
