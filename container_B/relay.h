#ifndef RELAY_h
#define RELAY_h 1

#define MAX_REQUESTS 10

enum relayMessage{
	FIND_MSG = 100,
	READY_MSG,
	CLOSE_MSG,
	CLOSING_MSG,
	CONFIRM_MSG
};
typedef enum relayMessage relayItemType;

#endif