#ifndef SHAREDSTRUCTURES_H
#define SHAREDSTRUCTURES_H

#define DEFAULT_SERVER_LISTEN_PORT		50513

// 64KB Chunk Size
#define FILE_CHUNK_SIZE			64*1024

#define SHA1_BYTECOUNT			20

#define DOWNLOADRATE_HISTORY_SIZE	500


#define CONN_CONTROL_REQUEST_FILE_LIST				0x002
#define CONN_CONTROL_LIST_REQUEST_GRANTED			0x003
#define CONN_CONTROL_LIST_REQUEST_REJECTED			0x004

#define CONN_CONTROL_REQUEST_FILE_DOWNLOAD			0x008
#define CONN_CONTROL_FILE_REQUEST_GRANTED			0x009
#define CONN_CONTROL_FILE_REQUEST_REJECTED			0x010

#define CONN_CONTROL_REQUEST_PARTIAL_FILE_DOWNLOAD	0x016
#define CONN_CONTROL_FILE_COMPLETE			0x128


#define MS_TO_S(x) (((x)/1000)%60)
#define MS_TO_M(x) (((x)/(1000*60))%60)
#define MS_TO_H(x) (((x)/(1000*60*60))%60)

enum ConnectionControl { REQUEST_FILE_LIST = 1,
						 LIST_REQUEST_GRANTED = (1<<1),
						 LIST_REQUEST_REJECTED = (1<<2),
						 LIST_REQUEST_EMPTY = (1<<2)+1,

						 REQUEST_FILE_DOWNLOAD = (1<<3),
						 FILE_DOWNLOAD_REQUEST_GRANTED = (1<<4),
						 FILE_DOWNLOAD_REQUEST_REJECTED = (1<<5),

						 REQUEST_PARTIAL_FILE = (1<<6),
						 PARTIAL_FILE_REQUEST_GRANTED = (1<<7),
						 PARTIAL_FILE_REQUEST_REJECTED = (1<<8),

						 FILE_COMPLETED = (1<<9),
						 FILE_CANCEL_TRANSFER = (1<<10)
					   };

struct connControlMsg
{
	ConnectionControl message;
	unsigned long long pos;
	char sha1_id[SHA1_BYTECOUNT];
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
