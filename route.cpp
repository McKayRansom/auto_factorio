/*
 * route.cpp
 * A basic maze router for a factorio belt layout
 * Inputs: doRouting takes netlist and routes it
 *
 *
 * McKay Ransom
 * 10/16/19
 */
#include <stdio.h> //printf
#include <string.h> //memcpy
#include <climits> //INT_MAX
#include <vector> //vector
#include "route.hpp"

#ifdef DEBUG_ROUTING
#define DEBUG_ROUTING_TRUE 1
#else
#define DEBUG_ROUTING_TRUE 0
#endif
// debug printing from: https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
#define debug_print_routing(...) \
    do { if (DEBUG_ROUTING_TRUE)\
        printf(__VA_ARGS__); } while (0)

/* 
 * valid_point - checks if a point is within the bounds of a map
 * TODO: consider making a macro or inline?
 */
bool valid_point (struct tile_map map, struct point p)
{
    return (p.y >= 0) && (p.y < map.size_y) && 
        (p.x >= 0) && (p.x < map.size_x );
}

/* 
 * valid_point_in_dir - checks if a point is valid a distance in a certain direction
 */
bool valid_point_in_dir (struct tile_map map, struct point p, int distance, struct direction dir)
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
struct tile tile_at(struct tile_map map, struct point location)
{
    if (!valid_point(map, location)) {
        printf("ERROR! attempting to access invalid point: %d %d\n", location.x, location.y);
        return map.tiles[0];
    }
    return map.tiles[location.x + (location.y * map.size_x)];
}

/*
 * pointer_tile_at - returns a pointer to a place on the map
 * Used if you need to modify the map
 */
struct tile* pointerToTileAt(struct tile_map map, struct point location)
{
    if (!valid_point(map, location)) {
        printf("ERROR! attempting to access invalid point: %d %d\n", x, y);
        return NULL;
    }
    return map.tiles + location.x + (location.y * map.size_x);
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
    } else if (myTile.belt_endpoint >= 0){
        printf("%i!", myTile.belt_endpoint);
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
void print_map(struct tile_map map)
{
    for (int i = 0; i < map.size_y; i++) {
        printf("|");
        for (int j = 0; j < map.size_x; j++) {
            struct tile my_tile = tile_at(map, {i, j});
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
void printcosts(struct tile_map map)
{
    printf("debug costs tile map:\n");
    for (int i = 0; i < map.size_y; i++) {
        printf("|");
        for (int j = 0; j < map.size_x; j++) {
            printf("-----");
        }
        printf("|\n|");
        for (int j = 0; j < map.size_x; j++) {
            struct tile my_tile = map_at(map, {i, j});
            int cost = INT_MAX;
            printf("%2d%2d|" , 
                    this_tile.costs[NORTH] == INT_MAX ? 0: this_tile.costs[NORTH],
                    this_tile.costs[EAST] == INT_MAX ? 0: this_tile.costs[EAST]);
        }
        printf("|\n|");
        for (int j = 0; j < size_x; j++) {
            struct tile this_tile = map_at(map, {i, j});
            printf("%2d%2d|" , 
                    this_tile.costs[SOUTH] == INT_MAX ? 0: this_tile.costs[SOUTH],
                    this_tile.costs[WEST] == INT_MAX ? 0: this_tile.costs[WEST]);
        }
        printf("|\n");
    }
}

/* 
 * translate - translates a point in a direction
 */
struct point translate(struct point start, struct direction dir)
{
    struct point result = start;
    switch(dir) {
    case NORTH:
        result.y = y - 1;
        break;
    case EAST:
        result.x = x + 1;
        break;
    case SOUTH:
        result.y = y + 1;
        break;
    case WEST:
        result.x = x - 1;
        break;
    default:
        printf("ERROR: invalid translation: %d attempted!\n", dir);
        break;
    }
}

/*
 * calc_cost - calculates the cost of a point
 * cost = 1 + number of adjacent endpoints + cost to get here
 */
int calc_cost(struct tile_map map, int belt_id, struct point this_point, int prior_cost)
{
    enum direction dirs_to_try[] = {NORTH, EAST, SOUTH, WEST};
    int cost = 1;
    for (int i = 0; i < 4; i ++) {
        struct point p = translate(this_point, dirs_to_try[i]);
        if (valid_point(map, p)) {
            struct tile this_tile = tile_at(map, p);
            if (this_tile.belt_endpoint >= 0 && this_tile.belt_endpoint != belt_id) {
                cost += 1;
            }
        }
    }
    //debug_print_routing("Cost of: %d, %d is %d (additional)\n", thisPoint.x, thisPoint.y, cost);
    return cost + prior_cost;
}

/* DEPRECATED???
 * obstacle_ahead - detects if there is an obstacle in the way (directly ahead) and space on the other side
bool obstacleAhead(struct tile* map, int size_x, int size_y, int belt_id, int x, int y, int direction) {
    // spot to check for obstacles
    int obstacleX, obstacleY;
    // tunnel exit
    int exitX, exitY;
    translate(x, y, direction, &obstacleX, &obstacleY);
    translate(obstacleX, obstacleY, direction, &exitX, &exitY);
    // option to go underground (only if obstacle and exit is free)
    if(valid_point(exitX, exitY, size_x, size_y)) {
        struct tile obstacleTile = tile_at(map, size_x, size_y, obstacleX, obstacleY);
        bool thing2 =(obstacleTile.hasBelt || obstacleTile.obstacle || 
            obstacleTile.belt_endpoint >= 0 && obstacleTile.belt_endpoint != belt_id) ;

    //printf("Obstacle ahead at: %d, %d in dir: %d with size: %dx%d thing1: %d thing2: %d obstacle at: %d, %d\n", x, y, direction, size_x, size_y, thing1, thing2, obstacleX, obstacleY);

    if (thing2) {
        return true;
    }
    }
    return false;
}*/


/* 
 * find_next_point - calculates the next point in a retrace
 */
struct retrace_point find_next_point(struct tile_map map, int belt_id, struct retrace_point start) {
    // direction order is VERY important! it must be consistent
    // first directino checked is the current direction (but we are looking backwards
    enum directions[] = {(start.dir) , SOUTH, WEST, NORTH, EAST};
    int num_directions = 5;
    // underground belts can't turn! (And the first point on a traceback already knows the direction)
    if (start.under || start.going_down || start.going_up) {
        num_directions = 1;
    }
    int cost = INT_MAX;
    int target_cost = (start.cost - (calc_cost(map, belt_id, start.pos, start.cost) - start.cost));
    struct retrace_point next_point = start;
    next_point.under = false;
    struct tile this_tile = tile_at(map, start.pos);
    // go through each direction in the order given
    debug_print_routing("Find Cost at: %d, %d currentCost: %d, numDirs: %d\n", start.pos.x, start.pos.y, start.cost, num_directions);
    if (target_cost < 0) {
        return 0;
    }
    for (int i =0; i < num_directions; i++) {
        enum direction this_dir = directions[i];
        // find direction that has the expected cost
        
        cost = this_tile.costs[this_dir];
        if (cost == start.cost) {
            if (direction == (start.dir + 2) % 4) {
                //don't allow doubling back!!
                //printf("WARNING, tried to double back!");
                continue;
            }     
            next_point.pos = translate(startingPoint.x, startingPoint.y, (direction + 2) % 4);
            if (valid_point(map, next_point.pos)) {
                struct tile target_tile = tile_at(map, next_point.pos);
                //TODO: reimplement this
                //if (target_tile.hasBelt || target_tile.obstacle ||
                    //(target_tile.belt_endpoint >= 0 && target_tile.belt_endpoint != belt_id)){ //a different belt's endpoint
                    //*isUnder = true;
                //}
            }
            break;
        }
    }
    return targetCost;
}
// check if a point is unoccupied/a fully valid point
// 0 for solution, 1 for empty, -1 for invalid
bool checkPoint(struct tile* map, int belt_id, int size_x, int size_y, struct toCheck current_point) {
    int startX = current_point.x;
    int startY = current_point.y;
    struct tile currentTile = map[startX + startY * size_x];

    //off the edge of map, these are supposed to be detecting when adding options initially
    if (!valid_point(startX, startY, size_x, size_y)) {
        printf("WARNING: invalid point attempted: %d, %d\n", startX, startY);
        return false;
    } else {
        //(whichBelt is used to indicate already searched)
        if (currentTile.belt_endpoint >= 0) {
            if (currentTile.belt_endpoint != belt_id && !current_point.underground) {//a different belt's endpoint
                return false;
            } else {
                return true;
            }
        }
        if ((!current_point.underground) &&
                    (currentTile.hasBelt ||    //belt here
                    currentTile.obstacle)) { //an obstacle
            //this tile is occupied already
            return false;
        }// else if (currentTile.whichBelt == belt_id && (!(current_point.goingDown || current_point.goingUp))) {
            //we have already checked this spot
            //return false;
        //}
    }
    //this is a free point
    return true;
}
// TODO: REMOVE THIS POS
void addBeginningTunnels(struct tile* map, std::vector<struct toCheck>* to_check_list, int size_x, int size_y, int belt_id, struct toCheck current_point) {
    int currentX = current_point.x;
    int currentY = current_point.y;
    int directions[] = {NORTH, EAST, SOUTH, WEST};
        int reverseDirection = (current_point.direction + 2) % 4;
        // STARTING POINT edge case. We want to consider every direction from the starting point
        if (current_point.direction < 0)
            reverseDirection = -1;
        //printf("Adding all options for %d, %d direction: %d (don't add dir %d)\n", currentX, currentY, current_point.direction, reverseDirection);
        // Check all directions
        for (int i = 0; i < 4; i++) {
            int newX, newY;
            translate(currentX, currentY, directions[i], &newX, &newY);
                    if (obstacleAhead(map, size_x, size_y, belt_id, currentX, currentY, directions[i])) {
                        to_check_list->push_back({newX, newY, directions[i], true, false, false, current_point.cost});
                        //                                                                                            underground under up
                    }
        }
}

/*
 * point_underground - returns bool true if point is blocked
 */
bool point_underground(struct tile_map map, struct point point, int belt_id)
{
    struct tile this_tile = point_at(map, point);
    if (this_tile.is_obstacle || (this_tile.net_endpoint && this_tile.net_endpoint != belt_id))
        return true;
    return false;
}

/*
 * add_to_list - add item to a linked list of point_to_check
 */
struct point_to_check* add_to_list(struct point_to_check* list, struct point_to_check item)
{
    if (list->next != NULL)
        printf("warning trying to add to non-empty end of list!");

    list->next = malloc(sizeof(struct point_to_check));
    *(list->next) = item;
    return list->next;
}
// add all possible routes from the current point to the to_check_list
// The routes are checked for validity but not for lack of collissions
struct point_to_check* add_all_options(struct tile_map map, struct point_to_check* to_check_list, int belt_id, struct point_to_check current_point) {
    //debug_print_routing("Adding options Aboveground at: %d, %d, dir:%d\n", currentX, currentY, current_point.direction);
    enum direction directions[] = {NORTH, EAST, SOUTH, WEST};
    enum direction rejerseDirection = (current_point.direction + 2) % 4;
    //printf("Adding all options for %d, %d direction: %d (don't add dir %d)\n", currentX, currentY, current_point.direction, reverseDirection);
    // Check all directions
    for (int i = 0; i < 4; i++) {
        // don't add a doubling-back belt (this is impossible!)
        if (directions[i] == reverseDirection ||
            // don't add turn belt if underground
            (current_point.going_up && directions[i] != current_point.direction)
        continue;

        // spot the new belt would go
        struct point new_point = translate(current_point.pos, directions[i]);
        // stay above ground
        if (!valid_point(newX, newY, size_x, size_y))
            continue;
        bool underground = point_underground(map, new_point, belt_id);
        bool straight = current_point.direction == directions[i];
        bool going_up = current_point.under && !underground;
        // do not add turns when going down
        if (underground && !straight)
            continue;
        // add to list                              position     direction      straight    underground      going up underlength cost  next pointer
        to_check_list = add_to_list(to_check_list, ({new_point, directions[i], straight, point_underground, going_up, 0, current_point.cost, NULL}));
    }
}

//*********************************************
// Routing Function
// routes a connection from start to end and returns cost
// Currently does a basic maze try every route strategy
int routeNet(struct tile* map, int belt_id, int size_x, int size_y, struct net net) {
    //reset map costs
    for (int i =0; i < size_x * size_y; i ++ ) {
        map[i].costs = (int[4]) {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    }

    //if the starting point is invalid!
    if (!checkPoint(map, belt_id, net.start)){
        printf("ERROR: routing of net: %d started on invalid point: %d, %d!\n", belt_id, net.start.x, net.start.y);
        print_map(map);
        return -1;
    } 

    // create starting point
    struct point_to_check* list_head;
    struct point_to_check* list_tail;
    // add all directions from starting point
    for (int i = 0; i < 4; i++) {
        struct point_to_check* new_point = malloc(sizeof(struct point_to_check));
        *(new_point) = (struct point_to_check) {net.start, i, true, false, false, 0, 1, NULL};
        if (list_head == NULL) {
            list_head = new_point;
        } else if (list_tail == NULL) {
            list_tail = new_point;
        } else {
            list_tail->next = new_point;
        }
    }

    // set costs of starting pont to 1
    pointer_to_tile_at(map, net.start)->costs = (int[4]) {1, 1, 1, 1};
 
    // add all starting tiles to visit
    addBeginningTunnels(map, &to_check_list, size_x, size_y, belt_id, startingPoint);
    add_all_options(map, &to_check_list, size_x, size_y, belt_id, startingPoint);

    struct point_to_check soltution_point;
    solution_point.cost = -1;
    // number of extra steps after finding solution to run (to find a more optimal solution)
    float percentAdditionalLoops = .1;
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

        //check next point
        struct point_to_check current_point = *list_head;
        free(list_head);
        
        //printf("Routing: %u, %u, under:%d, going:%d\n", currentX, currentY, current_point.underground, current_point.goingUnder);
        // if this is a valid point
        if (checkPoint(map, belt_id, size_x, size_y, current_point)){
            current_point.cost = calc_cost(map, size_x, size_y, belt_id, current_point);
            if (current_point.cost == INT_MAX) {
                printf("ERROR: invalid cost at %i, %i\n", current_point.x, current_point.y);
                print_costs(map, size_x, size_y);

            }
            // this is our solution! (if it isn't underground) TODO: WHY weird going down edge case? (A: it's because we check going down first...
            if (current_point.x == net.endX && current_point.y == net.endY && (!current_point.underground) && !current_point.goingDown) {
                // only accept better solutions
                if (finalPoint.cost > current_point.cost || finalPoint.cost < 0) {
                    //found solution!!
                    finalPoint = current_point;
                    debug_print_routing("*** Found solution of cost: %d goingUp: %d direction: %d\n", current_point.cost, current_point.goingUp, current_point.direction);
                    //TODO: decide how to add this back in w/o breaking anything
                    additionalLoops = numLoops * percentAdditionalLoops; 
                }
            }    

            struct tile* this_tile = pointerToTileAt(map, size_x, size_y, current_point.x, current_point.y);

            /*
            if (current_point.goingDown) {
                debug_print_routing("Routing goingDown at: %i, %i cost: %i\n", current_point.x, current_point.y, current_point.cost);
            } else if (current_point.underground) {
                debug_print_routing("Routing underground at: %i, %i cost: %i\n",current_point.x, current_point.y, current_point.cost);
            } else if (current_point.goingUp) {
                debug_print_routing("Routing goingUp at: %i, %i cost: %i    \n", current_point.x, current_point.y, current_point.cost);
            } else {
                debug_print_routing("Routing aboveground at: %i, %i cost: %i    \n", current_point.x, current_point.y, current_point.cost);
            }*/
            // add all possible ways to go from here
            if (current_point.cost < this_tile->costs[current_point.direction]) {
                this_tile->costs[current_point.direction] = current_point.cost;
                add_all_options(map, &to_check_list, size_x, size_y, belt_id, current_point);
            }
        }
    }

    //**********************************
    // Retrace the route and mark it on map
    //
    if (finalPoint.cost != -1) {
        struct toCheck current_point = finalPoint;
        struct toCheck lastPoint = current_point;
        // start checking one over from final point
        debug_print_routing("**** starting retrace at: %d, %d facing: %d\n", current_point.x, current_point.y, current_point.direction);
        //current_point.un

        bool isThisFirstPoint = true;
        if (current_point.goingUp) {
            current_point.goingUp = false; //TODO: Why is this edge case a thing?
            //current_point.underground = true;
        }
        
        while (true) {
            //update last point
            lastPoint = current_point;
            // Find the cost of a new point and update
            current_point.cost = findCost(map, belt_id, size_x, size_y, current_point, &whichX, &whichY, &isUnder);
            
            //reverse the direeciton when retracing
            //current_point.direction = (current_point.direction + 2) % 4;
            struct tile* this_tile = map + current_point.x + current_point.y * size_x;
            
            if (isUnder    && !current_point.underground) {
                // GOING UP (Backwards)
                debug_print_routing("Retracing going up: %i, %i dir: %d cost:%i\n", current_point.x, current_point.y, current_point.direction, current_point.cost);
                //current_point.goingUp = true;
                current_point.underground = true;
                this_tile->underground = true;
                this_tile->hasBelt = true;
                this_tile->whichBelt = belt_id;
                this_tile->belt_dir = current_point.direction;
            } else if (isUnder && (current_point.underground)) {
                // UNDER GROUND
                debug_print_routing("Retracing underground %i, %i dir: %d cost:%i\n", current_point.x, current_point.y, current_point.direction, current_point.cost);
                current_point.underground = true;
                //current_point.goingUp = false;
            } else if (!isUnder && current_point.underground) {
                current_point.underground = true;
                debug_print_routing("Retracing underground %i, %i dir: %d cost:%i\n", current_point.x, current_point.y, current_point.direction, current_point.cost);
                current_point.goingDown = true;
                current_point.underground = false;
            } else if (!isUnder && current_point.goingDown) {
                // GOING DOWN (backwards)
                debug_print_routing("Retracing going down %i, %i dir: %d cost:%i\n", current_point.x, current_point.y, current_point.direction, current_point.cost);
                current_point.underground = false;
                current_point.goingDown = false;
                this_tile->hasBelt = true;
                this_tile->underground = true;
                this_tile->belt_dir = current_point.direction;
                this_tile->whichBelt = belt_id;
            } else {
                // ABOVE GROUND
                debug_print_routing("Retracing above ground %i, %i dir: %d cost:%i\n", current_point.x, current_point.y, current_point.direction, current_point.cost);
                current_point.goingDown = false;
                this_tile->whichBelt = belt_id;
                this_tile->hasBelt = true;
                this_tile->belt_dir = current_point.direction;
            }
            if (current_point.x == net.startX && current_point.y == net.startY)
                break; // found solution!!
            // update to the next point
            current_point.x = whichX;
            current_point.y = whichY;

            if (lastPoint.y < current_point.y) // we are moving south (backwards)
                current_point.direction = NORTH;
            if (lastPoint.x > current_point.x) // we are moving west (backwards)
                current_point.direction = EAST;
            if (lastPoint.y > current_point.y) // we are moving north (backwards)
                current_point.direction = SOUTH;
            if (lastPoint.x < current_point.x) // we are moving east (backwards)
                current_point.direction = WEST;
    
            if (current_point.cost == INT_MAX || current_point.cost >= lastPoint.cost) {
                printf("ERROR! couldn't retrace route from %d, %d to %d, %d cost from %d to %d\n", lastPoint.x, lastPoint.y, current_point.x, current_point.y, lastPoint.cost, current_point.cost);
                print_costs(map, size_x, size_y);
                return -1;
            }
        }
        // we have to do this at the end so we don't mess up the traceback
        /*
        map[net.startX + (net.endY * size_x)].whichBelt = belt_id;
        map[net.startX + (net.endY * size_x)].hasBelt = true;
        map[net.startX + (net.endY * size_x)].belt_dir = finalPoint.direction;
        if (finalPoint.goingUp) {
            map[net.endX + (net.endY * size_x)].underground = true;
        }*/

        // TODO: in the future a final belt direction will need to be specified I think

    }
    return finalPoint.cost; //will be -1 if no route was found
}

// do all of the routing on a list of nets
// returns cost of all routes
// right now we quit after the first success
// Also, what should this return?
int route(struct tile* map, int size_x, int size_y, struct net* nets, int numNets, int numAttempts) {
    debug_print_routing("***** Beginning Routing *****\n");
    if (numAttempts <= 0) {
        numAttempts = numNets; // must be less than numNets!!
    }
    bool routed = false;
    int startingPoint = 0;
    int totalCost;
    int bestCost = INT_MAX;
    struct tile mapSolution[size_x * size_y];
    //retry routings in different order until one works or we have tried them all
    //while(!routed && numAttempts-- > 0) { 
    while(numAttempts-- > 0) { 
        int i = startingPoint++;
        totalCost = 0;
        // clear map
        for (int i = 0; i < size_x * size_y; i++) {
            map[i].belt_id = -1;
            map[i].belt_dir = 0;
            map[i].underground = false;
            //map[i].costs = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
            map[i].belt_endpoint = -1;
            //map[i].obstacle = false; //TEMP! move this so we can add obstacles before routing!
        }    
        // set belt enpoints on the map
        for (int i = 0; i < numNets; i++) {
            map[nets[i].startX + nets[i].startY * size_x].belt_endpoint = i;
            map[nets[i].endX + nets[i].endY * size_x].belt_endpoint = i;
        }
        // route each net
        for (int count = 0; count < numNets; count++) {
            debug_print_routing("routing net: %d\n", i);
            int cost = routeNet(map, i, size_x, size_y, nets[i]);
#ifdef DEBUG_ROUTING
            print_map(map, size_x, size_y);
            print_costs(map, size_x, size_y);
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
            
            i = ((i + 1) % numNets);
        }
        if (routed) {
            debug_print_routing("Found successful routing order of totalCost: %d!\n", totalCost);
            if (totalCost < bestCost) {
                bestCost = totalCost;
                for (int i = 0; i < size_x * size_y; i++) {
                    mapSolution[i] = map[i];
                }
            }
            
        } else if(numAttempts > 0) {
            debug_print_routing("***** Trying new Routing order *******\n");
        }
    }
    if (bestCost == INT_MAX) {
        return -1;
    }
    for (int i = 0; i < size_x * size_y; i++) {
        map[i] = mapSolution[i];
    }
    return bestCost;
}

