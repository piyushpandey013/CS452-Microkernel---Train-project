#ifndef __CAB_SERVICE_H__
#define __CAB_SERVICE_H__

#include <location.h>

#define CAB_SERVICE "cab_service"
int CabRequest(struct location *pickup, struct location *dropoff, int tid);

#endif
