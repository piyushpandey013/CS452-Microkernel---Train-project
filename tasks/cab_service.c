#include <cab_service.h>
#include <name_server.h>
#include <train_controller.h>
#include <syscall.h>
#include <sensor.h>
#include <clist.h>

#include <track_node.h>

#include <term.h>
#include <string.h> //memset

#define NUM_CABS 2
#define NUM_REQUESTS 10

enum cab_event {
	CAB_REQUEST = 1,
	CAB_DISPATCH_ARRIVE,
	CAB_DISPATCH_COMPLETE
};

struct cli_cmd {
	enum cab_event command; // CAB_REQUEST, CAB_DISPATCH_ARRIVE, CAB_DISPATCH_COMPLETE

	int train; // required for dispatch_arrive and dispatch_complete

	// used for CAB_REQUEST (as start and end locations)
	struct location loc1;
	struct location loc2;

	int status; // in case it's an event, indicates the status of the event (success/failure)

};

struct cab_descriptor {
	int train;
	struct train_position position;
	struct location resting_location; // the cab's resting node/sensor

	struct dispatch_request * dispatch;

	int courier_tid;
};

struct dispatch_request {
	int start_time; // use this along with end_time to calculate total time this dispatch took. customer can pay based on this

	struct location pick_up;
	struct location drop_off;
};

struct cab_courier_msg {
	struct location loc;
	int event;
};

struct track_node * get_track();
static int train_controller_tid, main_tid, term_tid, clock_tid;
static struct track_node *track;

static struct cab_descriptor cab_descriptors[NUM_CABS];
static struct dispatch_request dispatch_requests[NUM_REQUESTS];

static struct clist free_cabs_list;
static struct cab_descriptor * free_cab_nodes[NUM_CABS + 1];

static struct clist free_dispatch_list;
static struct dispatch_request * free_dispatch_nodes[NUM_REQUESTS + 1];

void cab_route_courier_task() {
	int train_controller_tid = WhoIs(TRAIN_CONTROLLER);
	int cab_server = WhoIs(CAB_SERVICE);
	int sending_tid;

	int train;

	Receive(&sending_tid, (char*)&train, sizeof(train));
	Reply(sending_tid, (char*)&train, sizeof(train));

	while (1) {
//		Printf("cab %d courier now ready!\r", train);
		struct cab_courier_msg cab_msg;
		Receive(&sending_tid, (char*)&cab_msg, sizeof(cab_msg));
		Reply(sending_tid, (char*)&train, sizeof(train));

//		Printf("cab %d courier: got it, task %d! Routing!\r", train, sending_tid);
		struct cli_cmd reply;
		reply.command = cab_msg.event;
		reply.train = train;
		reply.status = TrainControllerRoute(train, cab_msg.loc.node, 1, train_controller_tid); // 1 means blocking

		if (reply.status >= 0) {
			Delay(4 * TICKS_PER_SEC, clock_tid);
		}

//		Printf("cab %d courier: returned from routing. informing cab service!\r", train);

		if (cab_msg.event != -1) {
			Send(cab_server, (char*)&reply, sizeof(reply), (char*)&reply, sizeof(reply));
		}

	}
}

static void cab_init(struct cab_descriptor *cab, int train) {
	cab->train = train;
	cab->courier_tid = Create(PRIORITY_NORMAL, &cab_route_courier_task);

	int garbage;
	Send(cab->courier_tid, (char*)&train, sizeof(train), (char*)&garbage, sizeof(garbage));
}

static void cabs_init() {
	clist_init(&free_cabs_list, (int*)free_cab_nodes, NUM_CABS + 1);
	clist_init(&free_dispatch_list, (int*)free_dispatch_nodes, NUM_REQUESTS + 1);


	int i =0;
	for (i = 0; i < NUM_REQUESTS; i++) {
		clist_push(&free_dispatch_list, (int)(&dispatch_requests[i]));
	}


	memset(cab_descriptors, 0, sizeof(cab_descriptors));

	cab_init(&cab_descriptors[0], 35);
	cab_descriptors[0].resting_location.node = A4;
	cab_descriptors[0].resting_location.offset = 0;
	AssertF(clist_push(&free_cabs_list, (int)&(cab_descriptors[0])) == 1, "could not push ");

	cab_init(&cab_descriptors[1], 41);
	cab_descriptors[1].resting_location.node = B16;
	cab_descriptors[1].resting_location.offset = 0;
	AssertF(clist_push(&free_cabs_list, (int)&(cab_descriptors[1])) == 1, "could not push ");

//	cab_init(&cab_descriptors[2], 37);
//	cab_descriptors[2].resting_location.node = A4;
//	cab_descriptors[2].resting_location.offset = 0;

}

static void CabRoute(struct cab_descriptor *cab, struct location *loc, enum cab_event event) {

	struct cab_courier_msg cab_msg;
	cab_msg.event = event;
	memcpy(&(cab_msg.loc), loc, sizeof(cab_msg.loc));

	int reply = 0;
	Send(cab->courier_tid,  (char*)&cab_msg, sizeof(cab_msg), (char*)&reply, sizeof(reply));

}

static void on_dispatch_complete(struct cab_descriptor *cab, int status) {

	// add cab to free list
	AssertF(clist_push(&free_cabs_list, (int)cab) == 1, "could not add cab to free list");
	AssertF(clist_push(&free_dispatch_list, (int)(cab->dispatch)) == 1, "could not add dispatch node back to free list");

	int elapsed = Time(clock_tid) - cab->dispatch->start_time;

	TermPrintf(term_tid, "Cab %d %s, completed drop off at %d! Service time = %d seconds\r",
			cab->train,
			(status >= 0 ? "successfully" : "UNSUCCESSFULLY"),
			cab->dispatch->drop_off.node,
			elapsed / TICKS_PER_SEC);

	cab->dispatch = 0;

}

static void on_dispatch_arrive(struct cab_descriptor *cab, int status) {

	if (status >= 0) {
		int now = Time(clock_tid);
		TermPrintf(term_tid, "Cab %d arrived for pick up at %d! Arrival elapsed time = %d seconds\r",
				cab->train,
				cab->dispatch->pick_up.node,
				(now - cab->dispatch->start_time) / TICKS_PER_SEC);

		// stop all other trains that're trying to find this guy to stop
		int i = 0;
		for (i = 0; i < NUM_CABS; ++i) {
			if (cab_descriptors[i].dispatch == cab->dispatch && cab_descriptors[i].train != cab->train) {
				TrainControllerRouteStop(cab_descriptors[i].train, train_controller_tid);
			}
		}

		// go to drop off location
		TermPrintf(term_tid, "Cab %d starting to drop off at location %s!\r", cab->train, track[cab->dispatch->drop_off.node].name);
		CabRoute(cab, &cab->dispatch->drop_off, CAB_DISPATCH_COMPLETE);
	} else {
//		TermPrintf(term_tid, "Cab %d returning to resting location.\r", cab->train);
//		CabRoute(cab, &cab->resting_location, -1);
	}

}

int dispatch_cabs(struct dispatch_request *req) {
	struct cab_descriptor *cab = 0;

	int retval = 0;

	while (clist_len(&free_cabs_list) > 0) {
		clist_pop(&free_cabs_list, (int*)((int)(&cab)));
		cab->dispatch = req;

		TermPrintf(term_tid, "Sending cab %d to pick up @ location %s\r", cab->train, track[req->pick_up.node].name);

		// go to the pick up location
		CabRoute(cab, &req->pick_up, CAB_DISPATCH_ARRIVE);
		retval ++;
	}

	return retval;
}

static struct cab_descriptor * get_cab_by_train(int train) {
	int i = 0;
	for (i = 0; i < NUM_CABS; i++) {
		if (cab_descriptors[i].train == train) return &cab_descriptors[i];
	}

	return 0;
}

void cab_service_task() {
	RegisterAs(CAB_SERVICE);

	train_controller_tid = WhoIs(TRAIN_CONTROLLER);
	main_tid = WhoIs("main_task");
	term_tid = WhoIs(TERM_SERVER);
	clock_tid = WhoIs(CLOCK_SERVER);

	cabs_init();

	track = get_track();

	while (1) {
		char msg[100];
		int sending_tid;
		Receive(&sending_tid, msg, sizeof(msg));

		struct cli_cmd* cmd = (struct cli_cmd *)msg;

		switch (cmd->command) {
			case CAB_REQUEST: {

				int reply = 1;
				if (clist_len(&free_cabs_list) > 0) {

					struct dispatch_request *dispatch = 0;
					clist_pop(&free_dispatch_list, (int*)((int)(&dispatch)));

					dispatch->start_time = Time(clock_tid);
					memcpy(&dispatch->pick_up, &cmd->loc1, sizeof(cmd->loc1));
					memcpy(&dispatch->drop_off, &cmd->loc2, sizeof(cmd->loc2));

					TermPrintf(term_tid, "Added customer to waiting list for pickup/dropoff at %s/%s. (%d waiting so far)\r", track[dispatch->pick_up.node].name, track[dispatch->drop_off.node].name, NUM_REQUESTS-clist_len(&free_dispatch_list));
					int total = dispatch_cabs(dispatch);
					TermPrintf(term_tid, "Dispatched %d cabs.\r", total);
				} else {
					TermPrint("Cab service is too busy. Try again later!\r", term_tid);
					reply = -1;
				}

				Reply(sending_tid, (char*)&reply, sizeof(reply));
			} break;

			case CAB_DISPATCH_ARRIVE: {

				int reply = 1;
				struct cab_descriptor *cab = get_cab_by_train(cmd->train);

				Reply(sending_tid, (char*)&reply, sizeof(reply));
				Printf("Cab %d arrived to customer %s!\r", cab->train, cmd->status >= 0 ? "SUCCESSFULLY" : "UNSUCCESSFULLY");
				on_dispatch_arrive(cab, cmd->status);

			} break;

			case CAB_DISPATCH_COMPLETE: {

				int reply = 1;
				struct cab_descriptor *cab = get_cab_by_train(cmd->train);

				Reply(sending_tid, (char*)&reply, sizeof(reply));
				Printf("Cab %d completed drop off!!\r", cab->train);
				on_dispatch_complete(cab, cmd->status);

			} break;

		}
	}
}

// called by main_task usually
int CabRequest(struct location *pickup, struct location *dropoff, int tid) {
	struct cli_cmd cmd;
	cmd.command = CAB_REQUEST;
	memcpy(&cmd.loc1, pickup, sizeof(cmd.loc1));
	memcpy(&cmd.loc2, dropoff, sizeof(cmd.loc2));

	int reply = 0;
	Send(tid, (char*)&cmd, sizeof(cmd), (char*)&reply, sizeof(reply));

	return reply;
}
