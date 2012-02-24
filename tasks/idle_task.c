#include <io.h> // Printf()
#include <syscall.h> // Exit()

void idle_task_entry() {
//	unsigned int n = 0;
	while (1) {
		// reset watch dog timer
//		*((volatile int*)(0x80940000)) = 0x5555;

//		if (n == 0) {
//			Printf("I");
//		}
//		n++;
	}

	Printf("IDLE TASK CALLING EXIT!\r\n");
	Exit();
}
