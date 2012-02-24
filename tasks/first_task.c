#include <syscall.h>
#include <name_server.h>

void name_server_entry();
void clock_server_entry();
void uart_server_entry();
void train_command_entry();
void sensor_server_entry();
void switch_server_entry();
void track_server_entry();
void term_server_entry();

void main_task_entry();
void stopping_distance_task();
void velocity_task_entry();
void acceleration_calib_entry();
void first_task_entry() {
	int tid = Create(PRIORITY_HIGHEST, &name_server_entry);
	AssertF(tid == NAME_SERVER_TID, "Incorrect name_server tid");

	tid = Create(PRIORITY_HIGHEST - 1, &uart_server_entry);
	AssertF(tid > 0, "Could not create uart_server");

	tid = Create(PRIORITY_HIGHEST - 1, &clock_server_entry);
	AssertF(tid > 0, "Could not create clock_server");

	tid = Create(PRIORITY_HIGHEST - 2, &train_command_entry);
	AssertF(tid > 0, "Could not create train_server");

	tid = Create(PRIORITY_HIGHEST - 2, &sensor_server_entry);
	AssertF(tid > 0, "Could not create sensor_server");

	tid = Create(PRIORITY_HIGHEST - 2, &switch_server_entry);
	AssertF(tid > 0, "Could not create switch_server");
	
	tid = Create(PRIORITY_NORMAL + 1, &term_server_entry);
	AssertF(tid > 0, "Could not create term_server");

	tid = Create(PRIORITY_HIGHEST - 2, &track_server_entry);
	AssertF(tid > 0, "Could not create switch_server");

	tid = Create(PRIORITY_NORMAL, &main_task_entry);
	AssertF(tid > 0, "Could not create main_task");

	//Create(PRIORITY_NORMAL - 1, &velocity_task_entry);
	//Create(PRIORITY_NORMAL, &stopping_distance_task);
	//Create(PRIORITY_NORMAL - 1, &acceleration_calib_entry);

	Exit();
}
