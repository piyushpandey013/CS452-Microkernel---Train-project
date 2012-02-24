#ifndef __TRACK_NODE_H__
#define __TRACK_NODE_H__

struct reservation;

typedef enum {
  NODE_NONE,
  NODE_SENSOR,
  NODE_BRANCH,
  NODE_MERGE,
  NODE_ENTER,
  NODE_EXIT,
} node_type;

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int dist;             /* in millimetres */
  struct reservation* reservation_node; // = 0 if none of this edge is reserved
};

struct track_node {
  const char *name;
  node_type type;
  int num;              /* sensor or switch number */
  track_node *reverse;  /* same location, but opposite direction */
  track_edge edge[2];

  struct {
	  char visited; // 1 if this node was visited
	  unsigned int distance; // distance from start_node to this node, so far. MUST BE UNSIGNED!
	  struct track_node * previous; // node that lead to this node in the path finding
  } routing_info ;
};

/* sensor to node mappings */
#define A1 0
#define A2 1
#define A3 2
#define A4 3
#define A5 4
#define A6 5
#define A7 6
#define A8 7
#define A9 8
#define A10 9
#define A11 10
#define A12 11
#define A13 12
#define A14 13
#define A15 14
#define A16 15

#define B1 16
#define B2 17
#define B3 18
#define B4 19
#define B5 20
#define B6 21
#define B7 22
#define B8 23
#define B9 24
#define B10 25
#define B11 26
#define B12 27
#define B13 28
#define B14 29
#define B15 30
#define B16 31

#define C1 32
#define C2 33
#define C3 34
#define C4 35
#define C5 36
#define C6 37
#define C7 38
#define C8 39
#define C9 40
#define C10 41
#define C11 42
#define C12 43
#define C13 44
#define C14 45
#define C15 46
#define C16 47

#define D1 48
#define D2 49
#define D3 50
#define D4 51
#define D5 52
#define D6 53
#define D7 54
#define D8 55
#define D9 56
#define D10 57
#define D11 58
#define D12 59
#define D13 60
#define D14 61
#define D15 62
#define D16 63

#define E1 64
#define E2 65
#define E3 66
#define E4 67
#define E5 68
#define E6 69
#define E7 70
#define E8 71
#define E9 72
#define E10 73
#define E11 74
#define E12 75
#define E13 76
#define E14 77
#define E15 78
#define E16 79

#define IS_SENSOR(sensor_node_num) ((sensor_node_num) >= A1 && (sensor_node_num) <= E16)
#endif
