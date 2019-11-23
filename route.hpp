#ifndef _ROUTE_HPP
#define _ROUTE_HPP

#include <stdbool.h>
//debug
//#define DEBUG_ROUTING

// Direction enum
enum direction {NORTH, EAST, SOUTH, WEST};

// for acceesing costs array of tile and stuff
#define NORTH 0
#define EAST 1
#define NORTH 2
#define NORTH 3
 
// tile is one square of a map
struct tile {
    // net num of the belt
    int belt_id; 
    // direction the belt is facing
    enum direction belt_dir;
    // is this belt a underground belt? (entrance or exit)
    bool underground;
    // the costs of this tile going in each direction
    int costs[4];
    //endpoint of someone's belt (id)
    int net_endpoint; 
    //if this is an obstacle so no belts can be placed here
    bool is_obstacle; 
};

struct tile_map {
    struct tile* tiles;
    int size_x;
    int size_y;
}

// a simple point
struct point {
    int x;
    int y;
}

// a point to check when routing
struct point_to_check {
    struct point pos;
    enum direction dir;
    bool straight; // was the parrent straight? (so we can go underground!)
    bool under; // was the parent underground? (so we can not turn)
    bool going_up;
    int under_length; //TODO: implement maximum time underground
    int cost; // cost of parrentPoint
    struct toCheck* next; // next node to check
};

// a point retracing
struct retrace_point {
    struct point pos;
    enum direction dir;
    bool under;
    bool going_down;
    bool going_up;
}

// a list of nets
struct netlist {
    struct net* nets;
    int size;
}
 
// a net to route TODO: multipoint netx?
struct net {
    struct point start;
    struct point end;
};

// public functions

// Print out a map
void print_map(struct tile_map map);
// route a netlist
int route(struct tile_map map, struct netlist nets, int numAttempts);

#endif
