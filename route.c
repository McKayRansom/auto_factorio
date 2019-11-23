/*
 * route.cpp
 * A basic maze router for a factorio belt layout
 * Inputs: doRouting takes netlist and routes it
 *
 * TODO: add sorted list, add errors, add random orders instead of just
 * starting at different points, maximum tunnel length
 *
 * Notes: final belt direction may need tweaking
 *
 * Algorithm: Maze router with additional cost for
 * points near other nets' endpoints. Somewhat slow but 
 * garunteed to find a solution if one exists (for a given belt)
 *
 * Because we cannot move placed belts, some solutions for routing
 * every belt will not be found. For this reason the router will try 
 * different ordering of nets.
 * 
 * McKay Ransom
 * 10/16/19
 */
#include <stdio.h>  //printf
#include <string.h> //memcpy
#include <limits.h> //INT_MAX
#include <stdlib.h> //malloc
#include "route.h"

#ifdef DEBUG_ROUTING
#define DEBUG_ROUTING_TRUE 1
#else
#define DEBUG_ROUTING_TRUE 0
#endif
// debug printing from: https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
#define debug_print_routing(...) \
    do { if (DEBUG_ROUTING_TRUE)\
        printf(__VA_ARGS__); \
    } while (0)

/* 
 * valid_point - checks if a point is within the bounds of a map
 * TODO: consider making a macro or inline?
 */
bool valid_point(struct tile_map* map, struct point p)
{
    return (p.y >= 0) && (p.y < map->size_y) && 
        (p.x >= 0) && (p.x < map->size_x );
}

/* 
 * valid_point_in_dir - checks if a point is valid a distance in a certain direction
 */
bool valid_point_in_dir(struct tile_map* map, struct point p, int distance, enum direction dir)
{
    switch (dir) {
    case NORTH:
        p.y -= distance;
        break;
    case EAST:
        p.x += distance;
        break;
    case SOUTH:
        p.y += distance;
        break;
    case WEST:
        p.x -= distance;
        break;
    default:
        break;
    }
    return valid_point(map, p);
}


/* 
 * tile_at - returns the tile at a place on the map
 * This is used to abstract away what map is
 * right now it is only a two dimensional array so it is annoying to index
 * TODO: remove error checking?
 */
struct tile tile_at(struct tile_map* map, struct point location)
{
    if (!valid_point(map, location)) {
        printf("ERROR! attempting to access invalid point: %d %d\n", location.x, location.y);
        return map->tiles[0];
    }
    return map->tiles[location.x + (location.y * map->size_x)];
}

/*
 * pointer_tile_at - returns a pointer to a place on the map
 * Used if you need to modify the map
 */
struct tile* pointer_to_tile_at(struct tile_map* map, struct point location)
{
    if (!valid_point(map, location)) {
        printf("ERROR! attempting to access invalid point: %d %d\n", location.x, location.y);
        return NULL;
    }
    return map->tiles + location.x + (location.y * map->size_x);
}

/*
 * print_tile - prints a single tile in a nice way
 * Belts are printed as > < / or ^
 * Under ground entrances are - ] _ or [
 * Endpoints are '!'
 */
void print_tile(struct tile myTile)
{
    if (myTile.belt_id >= 0) {
        printf("%i", myTile.belt_id);
        if (myTile.underground){
            switch(myTile.belt_dir) {
            case NORTH:
                printf("-");
                break;
            case EAST:
                printf("]");
                break;
            case SOUTH:
                printf("_");
                break;
            case WEST:
                printf("[");
                break;
            }
        } else {
            switch(myTile.belt_dir) {
            case NORTH:
                printf("^");
                break;
            case EAST:
                printf(">");
                break;
            case SOUTH:
                printf("/");
                break;
            case WEST:
                printf("<");
                break;
            }
        }
    } else if (myTile.net_endpoint >= 0){
        printf("%i!", myTile.net_endpoint);
    } else if (myTile.is_obstacle) {
        printf("+ ");
    } else {
        printf("  "); // empty space
    }
}
 
/*
 * print_map - prints a map in a human readable format
 * Example:
 * |1>|1>|1/|
 * |  |  |1/|
 * |  |  |1/|
 */
void print_map(struct tile_map* map)
{
    for (int i = 0; i < map->size_y; i++) {
        printf("|");
        for (int j = 0; j < map->size_x; j++) {
            struct point this_point = {j, i};
            struct tile my_tile = tile_at(map, this_point);
            print_tile(my_tile);
        }
        printf("|\n");
    }
}

/*
 * print_costs - prints the costs of all tiles on a map
 * Prints costs as Four numbers for each of the 4 directins
 * example:
 * ------
 * |1214|
 * | 4 5|
 * ------
 */
void print_costs(struct tile_map* map)
{
    printf("debug costs tile map:\n");
    for (int i = 0; i < map->size_y; i++) {
        printf("|");
        for (int j = 0; j < map->size_x; j++) {
            printf("-----");
        }
        printf("|\n|");
        for (int j = 0; j < map->size_x; j++) {
            struct point this_point = {j, i};
            struct tile this_tile = tile_at(map, this_point);
            printf("%2d%2d|" , 
                    this_tile.costs[NORTH] == INT_MAX ? 0: this_tile.costs[NORTH],
                    this_tile.costs[EAST] == INT_MAX ? 0: this_tile.costs[EAST]);
        }
        printf("|\n|");
        for (int j = 0; j < map->size_x; j++) {
            struct point this_point = {j, i};
            struct tile this_tile = tile_at(map, this_point);
            printf("%2d%2d|" , 
                    this_tile.costs[WEST] == INT_MAX ? 0: this_tile.costs[WEST],
                    this_tile.costs[SOUTH] == INT_MAX ? 0: this_tile.costs[SOUTH]);
        }
        printf("|\n");
    }
}

/* 
 * translate - translates a point in a direction
 */
struct point translate(struct point start, enum direction dir)
{
    struct point result = start;
    switch(dir) {
    case NORTH:
        result.y--;
        break;
    case EAST:
        result.x++;
        break;
    case SOUTH:
        result.y++;
        break;
    case WEST:
        result.x--;
        break;
    default:
        printf("ERROR: invalid translation: %d attempted!\n", dir);
        break;
    }
    return result;
}

/*
 * calc_cost - calculates the cost of a point
 * cost = 1 + number of adjacent endpoints + cost to get here
 */
int calc_cost(struct tile_map* map, struct point this_point, int belt_id, int prior_cost)
{
    enum direction dirs_to_try[] = {NORTH, EAST, SOUTH, WEST};
    int cost = 1;
    for (int i = 0; i < 4; i ++) {
        struct point p = translate(this_point, dirs_to_try[i]);
        if (valid_point(map, p)) {
            struct tile this_tile = tile_at(map, p);
            if (this_tile.net_endpoint >= 0 && this_tile.net_endpoint != belt_id) {
                cost += 1;
            }
        }
    }
    //debug_print_routing("Cost of: %d, %d is %d (additional)\n", thisPoint.x, thisPoint.y, cost);
    return cost + prior_cost;
}

/*
 * point_occupied - returns bool true if point is blocked
 * also used to detect a point needing to be underground
 */
bool point_occupied(struct tile_map* map, struct point point, int belt_id)
{
    struct tile this_tile = tile_at(map, point);
    if (    this_tile.belt_id >= 0 || 
            this_tile.is_obstacle || 
            ((this_tile.net_endpoint >= 0) && (this_tile.net_endpoint != belt_id)))
        return true;
    return false;
}

/* 
 * retrace_next_point - calculates the next point in a retrace
 * also 'returns' going_down if there is a transition below ground
 */
struct retrace_point retrace_next_point(struct tile_map* map, int belt_id, struct retrace_point current, bool* going_down) 
{
    // direction order is VERY important! it must be consistent
    // first direction checked is the current direction (but we are looking backwards
    enum direction directions[] = {(current.dir) , SOUTH, WEST, NORTH, EAST};
    int num_directions = 5;
    // underground belts can't turn! (And the first point on a traceback already knows the direction)
    if (current.under)
        num_directions = 1;
    int target_cost = (current.cost - (calc_cost(map, current.pos, belt_id, current.cost) - current.cost));
    struct retrace_point next = current;
    struct tile this_tile = tile_at(map, current.pos);
    // go through each direction in the order given
    debug_print_routing("Find Cost at: %d, %d currentCost: %d, numDirs: %d\n", current.pos.x, current.pos.y, current.cost, num_directions);
    if (target_cost < 0) {
        return next;
    }
    // find the direction that has the expected cost
    for (int i =0; i < num_directions; i++) {
        enum direction this_dir = directions[i];
        if (this_tile.costs[this_dir] != current.cost)
            continue;

        // don't allow doubling back
        if (this_dir == (current.dir + 2) % 4)
            continue;

        next.pos = translate(current.pos, (this_dir + 2) % 4);
        if (!valid_point(map, next.pos))
            continue;
        
        // update this point
        next.under = point_occupied(map, next.pos, belt_id);
        *going_down = !current.under && next.under;
        next.dir = this_dir;
        next.cost = target_cost;
        break;
    }
    return next;
}

/*
 * retrace - retraces a cost map back from end to start
 * @map: cost map of all points
 * @solution_point: the last point in the solution
 * returns true if sucessful
 */
bool retrace(struct tile_map* map, struct point_to_check solution_point, struct point start, int belt_id)
{
    struct retrace_point current_point;
    current_point.pos = solution_point.pos;
    current_point.dir = solution_point.dir;
    current_point.cost = solution_point.cost;
    current_point.under = false;
    struct retrace_point last_point = current_point;
    debug_print_routing("**** starting retrace at: %d, %d facing: %d\n", current_point.pos.x, current_point.pos.y, current_point.dir);
    bool going_down;
    bool finished = false;
    
    while (!finished) {
        finished = current_point.pos.x == start.x && current_point.pos.y == start.y;

        // Calculate the next retrace point
        struct retrace_point next_point = retrace_next_point(map, belt_id, current_point, &going_down);
        debug_print_routing("Retracing at: %i, %i dir: %d cost:%i under: %d transition: %d\n", current_point.pos.x, current_point.pos.y, current_point.dir, current_point.cost, current_point.under, going_down);

        struct tile* this_tile = pointer_to_tile_at(map, current_point.pos);
        // cannot go down on the last point 
        // (sometimes we try to because of how the next point algorithm works)
        if (finished)
            going_down = false;
        // edge case: tunnel of length 1, we want to stay underground
        if (going_down && last_point.under) {
            current_point.under = true;
            going_down = false;
        }
        bool going_up = (last_point.under && !current_point.under);
        if (going_up || going_down) {
            // transitioning above/below ground
            this_tile->underground = true;
            this_tile->belt_id = belt_id;
            this_tile->belt_dir = current_point.dir;
        } else if (!current_point.under) {
            // above ground
            this_tile->belt_id = belt_id;
            this_tile->belt_dir = current_point.dir;
        }
        last_point = current_point;
        current_point = next_point;
        // error condition
        if (current_point.cost == INT_MAX || current_point.cost >= last_point.cost) {
            printf("ERROR! couldn't retrace route from %d, %d to %d, %d cost from %d to %d\n", last_point.pos.x, last_point.pos.y, current_point.pos.x, current_point.pos.y, last_point.cost, current_point.cost);
            print_costs(map);
            return false;
        }
    }
    return true;
}

/*
 * add_to_list - add item to a linked list of point_to_check
 * TODO: switch to sorted list (sorted by distance from endpoint somehow)
 */
struct point_to_check* add_to_list(struct point_to_check* list, struct point_to_check item)
{
    if (list == NULL) {
        list = malloc(sizeof(struct point_to_check));
        *list = item;
        return list;
    } else {
        list->next = malloc(sizeof(struct point_to_check));
        *(list->next) = item;
        return list->next;
    }
}

/* 
 * add_all_options - add all possible routes from the current point
 * The routes are checked for validity and should be valid
 * @to_check_list: linked list to add to
*/ 
struct point_to_check* add_all_options(struct tile_map* map, struct point_to_check* to_check_list, int belt_id, struct point_to_check current_point)
{
    //debug_print_routing("Adding options Aboveground at: %d, %d, dir:%d\n", currentX, currentY, current_point.direction);
    enum direction directions[] = {NORTH, EAST, SOUTH, WEST};
    enum direction reverse_direction = (current_point.dir + 2) % 4;
    debug_print_routing("Adding all options for %d, %d direction: %d (don't add dir %d)\n", current_point.pos.x, current_point.pos.y, current_point.dir, reverse_direction);
    // Check all directions
    for (int i = 0; i < 4; i++) {
        // don't add a doubling-back belt (this is impossible!)
        if (directions[i] == reverse_direction ||
            // don't add turn belt if underground
            ((current_point.going_up || current_point.under) && (directions[i] != current_point.dir)))
        continue;

        struct point_to_check new_point;
        new_point.dir = directions[i];
        // spot the new belt would go
        new_point.pos = translate(current_point.pos, new_point.dir);
        // stay above ground
        if (!valid_point(map, new_point.pos))
            continue;
        new_point.under = point_occupied(map, new_point.pos, belt_id);
        new_point.straight = current_point.dir == new_point.dir;
        new_point.going_up = current_point.under && !new_point.under;
        // do not add turns when going down
        if (new_point.under && !new_point.straight)
            continue;
        new_point.cost = current_point.cost;
        new_point.next = NULL;
        // add to list                              position     direction      straight    underground      going up underlength cost  next pointer
        to_check_list = add_to_list(to_check_list, new_point);
    }
    return to_check_list;
}

/*
 * route_net - routes one net from start to finish
 * Does a basic maze routing strategy
 * returns the cost of the route
 */
int route_net(struct tile_map* map, int belt_id, struct net net)
{
    //reset map costs
    for (int i =0; i < map->size_x * map->size_y; i ++ ) {
        map->tiles[i].costs[0] = INT_MAX;
        map->tiles[i].costs[1] = INT_MAX;
        map->tiles[i].costs[2] = INT_MAX;
        map->tiles[i].costs[3] = INT_MAX;
    }

    //if the starting point is invalid!
    if (point_occupied(map, net.start, belt_id)){
        printf("ERROR: routing of net: %d started on invalid point: %d, %d!\n", belt_id, net.start.x, net.start.y);
        print_map(map);
        return -1;
    } 

    // create list of points to check
    struct point_to_check* list_head = NULL;
    struct point_to_check* list_tail = NULL;
    // add all directions from starting point
    for (int i = 0; i < 4; i++) {
        struct point_to_check* new_point = malloc(sizeof(struct point_to_check));
        *(new_point) = (struct point_to_check) {net.start, i, true, false, false, 0, 0, NULL};
        if (list_head == NULL) {
            list_head = new_point;
        } else if (list_tail == NULL) {
            list_tail = new_point;
            list_head->next = new_point;
        } else {
            list_tail->next = new_point;
            list_tail = new_point;
        }
    }

    struct point_to_check solution_point;
    solution_point.cost = -1;
    // number of extra steps after finding solution to run (to find a more optimal solution)
    float percentAdditionalLoops = .5;
    int additionalLoops = -1;
    int numLoops = 0;
    // *************************************************
    // main Routing loop
    //
    while (list_head != NULL) {
        numLoops++;
        // TODO: FIND better solution than this??????
        // This doesn't work because it might overwrite the current solution,
        // so we should maybe copy the map and continue
        if (additionalLoops >= 0) {
            if (additionalLoops == 0) {
                debug_print_routing("routing loop timed out additional loops\n");
                break;
            } else {
                additionalLoops --;
            }
        }

        //check next point and free memory
        struct point_to_check current_point = *list_head;
        
        debug_print_routing("Routing: %u, %u, under:%d, going:%d\n", current_point.pos.x, current_point.pos.y, current_point.under, current_point.going_up);
        current_point.cost = calc_cost(map, current_point.pos, belt_id, current_point.cost);
        if (current_point.cost == INT_MAX) {
            printf("ERROR: invalid cost at %i, %i\n", current_point.pos.x, current_point.pos.y);
            print_costs(map);
        }
        // this is our solution! (if it isn't underground) TODO: WHY weird going down edge case? (A: it's because we check going down first...
        if (current_point.pos.x == net.end.x && current_point.pos.y == net.end.y && (!current_point.under)) {
            // only accept better solutions
            if (solution_point.cost > current_point.cost || solution_point.cost < 0) {
                //found solution!!
                solution_point = current_point;
                debug_print_routing("*** Found solution of cost: %d goingUp: %d direction: %d\n", current_point.cost, current_point.going_up, current_point.dir);
                //TODO: decide how to add this back in w/o breaking anything
                additionalLoops = numLoops * percentAdditionalLoops; 
            }
        }    

        if (current_point.under) {
            debug_print_routing("Routing underground at: %i, %i cost: %i\n",current_point.pos.x, current_point.pos.y, current_point.cost);
        } else if (current_point.going_up) {
            debug_print_routing("Routing goingUp at: %i, %i cost: %i    \n", current_point.pos.x, current_point.pos.y, current_point.cost);
        } else {
            debug_print_routing("Routing aboveground at: %i, %i cost: %i    \n", current_point.pos.x, current_point.pos.y, current_point.cost);
        }
        struct tile* this_tile = pointer_to_tile_at(map, current_point.pos);
        // only proceed if this way to get to this point has a lower cost
        if (current_point.cost < this_tile->costs[current_point.dir]) {
            // valid tile so add all possibilites from here
            this_tile->costs[current_point.dir] = current_point.cost;
            list_tail = add_all_options(map, list_tail, belt_id, current_point);
        }

        // free memory of the point we just checked
        struct point_to_check* old_point = list_head;
        list_head = old_point->next;
        free(old_point);
    }

    // free list
    while (list_head != NULL) {
        struct point_to_check* temp = list_head;
        list_head = list_head->next;
        free(temp);
    }

    // if a solution exists, retrace the route and mark it on map
    if (solution_point.cost != -1) {
        if (retrace(map, solution_point, net.start, belt_id)) {
            return solution_point.cost;
        }
    }
    
    return -1; // return error
}

/*
 * route - route a netlist
 * routes nets one at a time without moving them
 * will try every sequential ordring of nets to find the best order
 * returns the total cost of all nets
 */
int route(struct tile_map* map, struct netlist this_netlist, int num_attempts)
{
    debug_print_routing("***** Beginning Routing *****\n");
    if (num_attempts <= 0) {
        num_attempts = this_netlist.size; // must be less than numNets!!
    }
    bool routed = false;
    int startingPoint = 0;
    int totalCost;
    int bestCost = INT_MAX;
    struct tile_map map_solution;
    struct tile tiles[map->size_x * map->size_y];
    map_solution.tiles = tiles;
    //retry routings in different order until one works or we have tried them all
    //while(!routed && num_attempts-- > 0) { 
    while(num_attempts-- > 0) { 
        int i = startingPoint++;
        totalCost = 0;
        // clear map
        for (int i = 0; i < map->size_x * map->size_y; i++) {
            map->tiles[i].belt_id = -1;
            map->tiles[i].belt_dir = 0;
            map->tiles[i].underground = false;
            //map[i].costs = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
            map->tiles[i].net_endpoint = -1;
            //map[i].obstacle = false; //TEMP! move this so we can add obstacles before routing!
        }    
        // set belt enpoints on the map
        for (int i = 0; i < this_netlist.size; i++) {
            pointer_to_tile_at(map, this_netlist.nets[i].start)->net_endpoint = i;
            pointer_to_tile_at(map, this_netlist.nets[i].end)->net_endpoint = i;
        }
        // route each net
        for (int count = 0; count < this_netlist.size; count++) {
            debug_print_routing("routing net: %d\n", i);
            int cost = route_net(map, i, this_netlist.nets[i]);
#ifdef DEBUG_ROUTING
            print_map(map);
            print_costs(map);
#endif
            if (cost < 0) { //routing failed
                debug_print_routing("routing net %d FAILED!\n", i);
                routed = false;
                break;
            } else {
                debug_print_routing("routing net %d Sucess! with cost: %d\n", i, cost);
                routed = true;
                totalCost += cost;
            }
            
            i = ((i + 1) % this_netlist.size);
        }
        if (routed) {
            debug_print_routing("Found successful routing order of totalCost: %d!\n", totalCost);
            if (totalCost < bestCost) {
                bestCost = totalCost;
                // TODO: memcopy?
                for (int i = 0; i < map->size_x * map->size_y; i++) {
                    map_solution.tiles[i] = map->tiles[i];
                }
            }
            
        } else if(num_attempts > 0) {
            debug_print_routing("***** Trying new Routing order *******\n");
        }
    }
    if (bestCost == INT_MAX) {
        return -1;
    }
    for (int i = 0; i < map->size_x * map->size_y; i++) {
        map->tiles[i] = map_solution.tiles[i];
    }
    return bestCost;
}

