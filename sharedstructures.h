#ifndef SHAREDSTRUCTURES_H
#define SHAREDSTRUCTURES_H

#define DEFAULT_SERVER_LISTEN_PORT		50513

// 64KB Chunk Size
#define FILE_CHUNK_SIZE			64*1024


#define CONN_CONTROL_REQUEST_FILE_LIST		0x10
#define CONN_CONTROL_REQUEST_FILE_DOWNLOAD	0x11
#define CONN_CONTROL_FILE_COMPLETE			0x12

struct connControlMsg
{
	unsigned int message;
	char sha1_id[20];
};

struct file
{
	struct
	{
		unsigned int name_len;
		unsigned int file_size;
		unsigned int file_id;
	} basic_info;

	char *file_name;
};

#endif
