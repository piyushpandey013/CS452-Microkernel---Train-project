#include <syscall.h>
#include <train_command.h>
#include <string.h>
#include <clist.h>
#include <sensor.h>

#include <name_server.h>
#include <clock.h>
#include <uart_server.h>

#define MAX_EVENT_QUEUE_SIZE 5

struct sensor_worker_message {
	char module_bytes[NUM_SENSOR_MODULES * 2];
	int timestamp;
};

void clock_helper_task_entry() {
	int parent_tid = MyParentTid();
	int clock_tid = WhoIs("clock_server");
	
	int msg;
	while (1) {
		Send(parent_tid, (char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg));
		Delay(msg, clock_tid);
	}
}

void getc_helper_task_entry() {
	int parent_tid = MyParentTid();
	int uart_tid = WhoIs("uart_server");
	
	int msg;
	while (1) {
		msg = Getc(COM1, uart_tid);
		Send(parent_tid, (char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg));
	}
}

void sensor_worker_task_entry() {
	int parent_tid = MyParentTid();
	int train_tid = WhoIs(TRAIN_COMMAND);
	int uart_tid = WhoIs(UART_SERVER);
	int clock_tid = WhoIs(CLOCK_SERVER);
	
	// Spawn some helper tasks so we can perform a timeout clearing of
	// the train controller's UART messages
	int clock_helper_tid = Create(PRIORITY_HIGHEST, &clock_helper_task_entry);
	int getc_helper_tid = Create(PRIORITY_HIGHEST, &getc_helper_task_entry);

	int reply = 1;
	int sender_tid;
	
	// Set up a delay of 3 seconds by replying the delay to the clock_helper_task
	int delay = 300;
	Receive(&sender_tid, (char*)&reply, sizeof(reply));
	AssertF(sender_tid == clock_helper_tid, "Expecting clock_helper_task to send message. Got message from %d", sender_tid);
	Reply(clock_helper_tid, (char*)&delay, sizeof(delay));

	// Keep reading bytes out of UART1 until our delay expires
	while (Receive(&sender_tid, (char*)&reply, sizeof(reply)) > 0 && sender_tid != clock_helper_tid) {
		AssertF(sender_tid == getc_helper_tid, "Expecting getc_helper_task to send message. Got message from %d", sender_tid);
		Reply(getc_helper_tid, (char*)&reply, sizeof(reply));
	}

	// Ask for sensor data and read it in. When all bytes have been received, send the data
	// to the sensor server for interpretation
	struct sensor_worker_message message;
	while (1) {
		char *p = message.module_bytes;
		
		TrainCommandResetSensors(train_tid);
		TrainCommandReadSensors(NUM_SENSOR_MODULES, train_tid);
		
		int i;
		for (i = 1; i <= NUM_SENSOR_MODULES*2; ++i, ++p) {
			*p = Getc(COM1, uart_tid);
		}
		
		// It takes us 60 milliseconds to read sensor data, so we should at least remove that
		// time from the current time
		message.timestamp = Time(clock_tid) - SENSOR_DELAY_AVG;

		// send sensor data to sensor server
		Send(parent_tid, (char*)&message, sizeof(message), (char*)&reply, sizeof(reply));
	}

	Exit();
}

void sensor_server_entry() {
	RegisterAs("sensor_server");
	
	// Initialize the sensor table
	char sensor_table[NUM_SENSOR_MODULES * NUM_SENSORS_PER_MODULE];
	memset(sensor_table, 0xff, sizeof(sensor_table));
	
	// Initialize the list of tid's to notify of sensor events
	struct clist event_list;
	int event_list_buffer[MAX_EVENT_QUEUE_SIZE];
	clist_init(&event_list, event_list_buffer, MAX_EVENT_QUEUE_SIZE);
	
	// Create the worker task that handles the collection of sensor data from the
	// Train Controller
	int worker_tid = Create(PRIORITY_HIGHEST - 1, &sensor_worker_task_entry);
	
	int reply;
	int sender_tid;
	struct sensor_worker_message message;
	while (1) {
		Receive(&sender_tid, (char*)&message, sizeof(message));
		
		if (sender_tid == worker_tid) {
			// Sensor worker is reporting sensor data. Reply so the sensor worker can get back to work
			Reply(worker_tid, (char*)&reply, sizeof(reply));

			struct sensor_report report;
			report.num_sensors = 0;

			// Process the data		
			int module;
			for (module = 0; module < NUM_SENSOR_MODULES; module++) {
				int module_data = (int)message.module_bytes[module * 2];
				module_data = module_data << 8;
				module_data |= message.module_bytes[(module * 2) + 1];
				
				// Update the entry for this module in the sensor table
				int sensor = NUM_SENSORS_PER_MODULE - 1;
				for (; sensor >= 0; sensor--) {
					const char is_sensor_on = (char)(module_data & 0x01);
					const int table_index = (sensor - 1) + (module * NUM_SENSORS_PER_MODULE);
					if (sensor_table[table_index] != 0xff && sensor_table[table_index] != is_sensor_on) {
						if (is_sensor_on) {
							// Notify interested tasks that this sensor got tripped
							report.sensor_nodes[report.num_sensors++] = SENSOR_TO_NODE(module, sensor);
						}
					}
					sensor_table[table_index] = (char)is_sensor_on;
					module_data = module_data >> 1;
				} // single module decoded
			} // all modules decoded

			report.timestamp = message.timestamp;

			if (report.num_sensors > 0) {
				int tid;
				while (clist_pop(&event_list, &tid) == 1) {
					Reply(tid, (char*)&report, sizeof(report));
				}
			}
		} else {
			// some task wants to listen to sensors. add him to event queue!
			AssertF(clist_push(&event_list, sender_tid) == 1, "Failed to register for sensor events");
		}
	}
}
// The courier task associated with this server
static void sensor_server_courier_entry() {
	int parent_tid = MyParentTid();
	int sensor_tid = WhoIs(SENSOR_SERVER);

	struct sensor_report msg;
	int phony;
	while (1) {
		AssertF(Send(sensor_tid, (char*)&phony, sizeof(phony), (char*)&msg, sizeof(msg)) > 0, "Could not listen to sensor events");
		Send(parent_tid, (char*)&msg, sizeof(msg), (char*)&phony, sizeof(phony));
	}
}

int SensorGetNextTriggered(int *sensor_node, int* time, int tid) {
	struct sensor_report report;
	int phony;

	if (Send(tid, (char*)&phony, sizeof(phony), (char*)&report, sizeof(report)) > 0) {
		*sensor_node = (int)report.sensor_nodes[0];
		*time = report.timestamp;
		return 0;
	}
	Assert("bad from sensor\r");
	return -1;
}

int CreateSensorCourier(int priority) {
	return Create(priority, &sensor_server_courier_entry);
}
