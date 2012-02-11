#ifndef FILEINFO_H
#define FILEINFO_H
#include <QString>
#include <QByteArray>
#include <QList>
#include <QUuid>

class FileInfo
{
public:
	FileInfo();
	FileInfo(int id, qint64 size, int isDir, int parentId, QString name);
	~FileInfo();

	void setSize(qint64 newSize);
	qint64 getSize() const;

	int getId() const;
	void setId(int newId);

	int isDir() const;
	void setDir(int isDir);

	int getParentId() const;
	void setParentId(int newParentId);

	void setName(const QString &name);
	QString getName() const;

	QByteArray getHash();


	QByteArray getByteArray() const;
	bool setFromByteArray(char *buff);

	//Server side functions
	void setPath(const QString &path);
	QString getPath() const;
	QList<FileInfo*> getChildList();

	//Tree Struct management
	FileInfo* child(unsigned int index);
	int childIndex() const;

	int childCount() const;

	void appendChild(FileInfo* child);
	void insertChild(unsigned int pos, FileInfo* child);
	void insertBlank(unsigned int pos, unsigned int count);

	void removeChild(unsigned int pos, unsigned int count);

	FileInfo *parent();


	//Static members for ID management
	int indexOfByID(int id);

	static int nextID();
	static void resetID();

private:

	//Shared members
	int m_id;
	qint64 m_size;
	int m_isDir;
	int m_parentId;
	QByteArray m_sha1_id;
	QString m_name;

	//Server-only
	QString m_path;

	//Tree Struct data keeping
	QList<FileInfo*> childItems;
	FileInfo* parentItem;

	//Static ID keeping
	static int m_nextID;	//Next ID will be 1, 0 is reserved
};

#endif // FILEINFO_H
