#ifndef __TERM_H__
#define __TERM_H__

#define CURSOR_RESET "\x1b[\x00m"
#define CURSOR_POS "\x1b[%d;%dH"
#define CURSOR_BACK "\x1b[%dD"
#define CURSOR_SAVE "\x1b[s"
#define CURSOR_RESTORE "\x1b[u"
#define CURSOR_CLEAR_SCREEN "\x1b[2J"
#define CURSOR_CLEAR "\x1b[J"
#define CURSOR_SCROLL "\x1b[%d;%dr"
#define CURSOR_CLEAR_LINE_AFTER "\x1b[K"

#define TERM_RESERVATION_LINE 18

#define TERM_PROMPT_TOP 20
#define TERM_PROMPT_BOTTOM 999

#define TERM_SENSOR_TOP_OFFSET 3
#define TERM_SENSOR_LEFT_OFFSET 3
#define TERM_SENSOR_BOTTOM_OFFSET 17

#define TERM_SENSOR_LINES (TERM_SENSOR_BOTTOM_OFFSET - TERM_SENSOR_TOP_OFFSET)
#define TERM_SENSOR_LINE_LENGTH 15

int TermPrint(char* message, int tid);
int TermPutc(char c, int tid);
void TermPrintf(int tid, char *fmt, ...);

#define TERM_SERVER "term_server"

#endif
