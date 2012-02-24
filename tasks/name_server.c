#include <bwio.h>

#include <syscall.h>
#include <name_server.h>
#include <task.h>
#include <string.h>

#define MAX_NAME_SIZE 128
#define MAX_MSG_SIZE 256
#define MAX_NUM_RECORDS MAX_NUM_TASKS

#define REQUEST_WHOIS 0
#define REQUEST_REGISTER 1

#define ERROR_NS_OUT_OF_RECORDS -1
#define ERROR_NS_BAD_REQUEST -3

/* Holds the mapping of name to tid */
struct name_record {
	char name[MAX_NAME_SIZE];
	int tid;
	struct name_record* next;
};

/* Message structs */
struct name_message {
	int type;
	char name[MAX_NAME_SIZE];
};

static void name_server_init(struct name_record* records, int len) {
	int i;
	/* Point each block to the next except the last one */
	for (i = 0; i < (len - 1); i++) {
		records[i].next = &records[i + 1];
	}
	records[i].next = 0;
}

struct name_record* name_server_whois(struct name_record* list, char* name) {
	while (list != 0) {
		if (strcmp(name, list->name) == 0) {
			return list;
		}
		list = list->next;
	}
	return 0;
}

int name_server_register(struct name_record** active_list, struct name_record** free_list, char* name, int sender_tid) {
	struct name_record* record = name_server_whois(*active_list, name);
	if (record == 0) {
		if (*free_list == 0) {
			return ERROR_NS_OUT_OF_RECORDS;
		}

		record = *free_list;
		*free_list = record->next;

		record->next = *active_list;
		*active_list = record;
	}
	
	// Copy name here
	memcpy(record->name, name, strlen(name) + 1);
	record->tid = sender_tid;
	return 0;
}

/************* DEBUGGING RELATED STUFF *******/
/* Allocate a chunk of stack space to the store name to tid mappings */
static struct name_record records[MAX_NUM_RECORDS];
static struct name_record *free_list, *active_list;

struct name_record * name_server_get_active_list() {
	return active_list;
}

/* Entry point of the name server */
void name_server_entry() {

//	bwprintf(COM2, "Name server initializing...\r\n");

	/* Intialize the free list */
	name_server_init(records, MAX_NUM_RECORDS);
	free_list = records;

	/* Initialize the active list */
	active_list = 0;
	
	int bytesRec;
	int sender_tid;
	char message[MAX_MSG_SIZE];
	int reply;
	while (1) {
		/* Blocks until a message is received */
		bytesRec = Receive(&sender_tid, message, MAX_MSG_SIZE);

		if (bytesRec <= MAX_MSG_SIZE) {
			/* The message fit in the buffer, so let's service it */
			struct name_message* name_message = (struct name_message*)message;
			switch(name_message->type) {
				case REQUEST_WHOIS: {
					struct name_record* record = name_server_whois(active_list, name_message->name);
					if (record == 0) {
						reply = 0;
					} else {
						reply = record->tid;
					}
					break;
				}

				case REQUEST_REGISTER:
					reply = name_server_register(&active_list, &free_list, name_message->name, sender_tid);
					break;

				default:
					reply = ERROR_NS_BAD_REQUEST;
					break;
			}
		}
		Reply(sender_tid, (char*)&reply, sizeof(int));
	}

//	bwprintf(COM2, "Name server exiting, which it shouldn't\r\n");
	Exit();
}

int WhoIs(char* name) {
	struct name_message msg;
	int status, reply;
	msg.type = REQUEST_WHOIS;
	memcpy(msg.name, name, strlen(name) + 1);
	status = Send(NAME_SERVER_TID, (char*)&msg, sizeof(struct name_message), (char*)&reply, sizeof(int));
	AssertF(reply > 0, "WhoIs fail: no server named %s", name);
	return reply;
}

int RegisterAs(char* name) {
	struct name_message msg;
	int status, reply;
	msg.type = REQUEST_REGISTER;
	memcpy(msg.name, name, strlen(name) + 1);
	status = Send(NAME_SERVER_TID, (char*)&msg, sizeof(struct name_message), (char*)&reply, sizeof(int));
	return reply;
}

