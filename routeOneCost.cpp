/* route.cpp
 * A basic maze router for a factorio belt layout
 * Inputs: doRouting takes netlist and routes it
 *
 *
 * McKay Ransom
 * 10/16/19
 */
//libraries
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

// prints a map tile Type (belt, etc)
void printTile(struct tile myTile) {
  if (myTile.hasBelt) {
    printf("%i", myTile.whichBelt);
    if (myTile.underground){
      switch(myTile.beltDirection) {
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
      switch(myTile.beltDirection) {
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
  } else if (myTile.beltEndpoint >= 0){
    printf("%i!", myTile.beltEndpoint);
  } else if (myTile.obstacle) {
    printf("+ ");
  } else {
    printf("  ");
  }
}
// prints costs on the map
void printCosts(struct tile* map, int sizeX, int sizeY) {
  printf("Debug costs map:\n");
  for (int i = 0; i < sizeY; i++) {
    printf("|");
    for (int j = 0; j < sizeX; j++) {
      printf("-----");
    }
    printf("|\n|");
    for (int j = 0; j < sizeX; j++) {
      struct tile myTile = map[(i*sizeX)+j];
      printTile(myTile);
      printf("%2d|" , myTile.cost == INT_MAX ? 0: myTile.cost);
    }
    printf("|\n|");
    for (int j = 0; j < sizeX; j++) {
      struct tile myTile = map[(i*sizeX)+j];
      printf(" %2d |" , myTile.underCost == INT_MAX ? 0: myTile.underCost);
    }
    printf("|\n");
  }
}
 
// prints a map in a mostly human readable format
void printMap(struct tile* map, int sizeX, int sizeY) {
  for (int i = 0; i < sizeY; i++) {
    printf("|");
    for (int j = 0; j < sizeX; j++) {
      struct tile myTile = map[(i*sizeX)+j];
      printTile(myTile);
    }
    printf("|\n");
  }
}
// translates a point in a cardinal direction
void translate (int x, int y, int dir, int* newX, int* newY) {
  switch(dir) {
    case NORTH:
      *newX = x;
      *newY = y - 1;
      break;
    case EAST:
      *newX = x + 1;
      *newY = y;
      break;
    case SOUTH:
      *newX = x;
      *newY = y + 1;
      break;
    case WEST:
      *newX = x - 1;
      *newY = y;
      break;
    default:
      printf("ERROR: invalid translation: %d attempted!\n", dir);
      break;
  }
}
//checks if a point is valid for a given grid
bool validPoint (int x, int y, int sizeX, int sizeY) {
  return (y >= 0) && (y < sizeY) && 
    (x >= 0) && (x < sizeX );
}
//above but only for one direction TODO: USE THIS
//checks if a point is valid for a given grid and distance from edge
bool validPointInDirection (int x, int y, int sizeX, int sizeY, int distance, int direction) {
  switch (direction) {
    case NORTH:
      y -= distance;
      break;
    case EAST:
      x += distance;
      break;
    case SOUTH:
      y += distance;
      break;
    case WEST:
      x -= distance;
      break;
  }
  return (y >= 0) && (y < sizeY) && 
    (x >= 0) && (x < sizeX );
}

// returns the tile at a certain place on map 
struct tile tileAt(struct tile* map, int sizeX, int sizeY, int x, int y) {
  if (!validPoint(x, y, sizeX, sizeY)) {
    printf("ERROR! attempting to access invalid point: %d %d\n", x, y);
    return map[0];
  }
  return map[x + (y * sizeX)];
}
// returns pointer to the tile
struct tile* pointerToTileAt(struct tile* map, int sizeX, int sizeY, int x, int y) {
  if (!validPoint(x, y, sizeX, sizeY)) {
    printf("ERROR! attempting to access invalid point: %d %d\n", x, y);
    return NULL;
  }
  return map + x + (y * sizeX);
}


// calculate the cost of a point
int calcCost(struct tile* map, int sizeX, int sizeY, int myBelt, struct toCheck thisPoint) {
  int directions[] = {NORTH, EAST, SOUTH, WEST};
  // base cost of 1
  int cost = 1;
  // add 1 for underground (to discourage long tunnels
  // TODO: REMOVE THIS TEMP FIX!!
  //if (thisPoint.underground) {
    //cost++;
  
  // add cost for adjacent endpoints
  for (int i = 0; i < 4; i ++) {
    int newX, newY;
    translate(thisPoint.x, thisPoint.y, directions[i], &newX, &newY);
    if (validPoint(newX, newY, sizeX, sizeY)) {
      struct tile thisTile = tileAt(map, sizeX, sizeY, newX, newY);
      if (thisTile.beltEndpoint >= 0 && thisTile.beltEndpoint != myBelt) {
        cost += 1;
      }
    }
  }
  debug_print_routing("Cost of: %d, %d is %d (additional)\n", thisPoint.x, thisPoint.y, cost);
  return cost + thisPoint.cost;
}

// detects if there is an obstacle in the way (directly ahead) and space on the other side
bool obstacleAhead(struct tile* map, int sizeX, int sizeY, int x, int y, int direction) {
  // spot to check for obstacles
  int obstacleX, obstacleY;
  // tunnel exit
  int exitX, exitY;
  translate(x, y, direction, &obstacleX, &obstacleY);
  translate(obstacleX, obstacleY, direction, &exitX, &exitY);
  // option to go underground (only if obstacle and exit is free)
  bool thing1 =validPoint(exitX, exitY, sizeX, sizeY);
  bool thing2 =(map[obstacleX + obstacleY * sizeX].hasBelt || map[obstacleX + obstacleY * sizeX].obstacle) ;

  //printf("Obstacle ahead at: %d, %d in dir: %d with size: %dx%d thing1: %d thing2: %d obstacle at: %d, %d\n", x, y, direction, sizeX, sizeY, thing1, thing2, obstacleX, obstacleY);

  if (thing1&& thing2) {
    return true;
  }
  return false;
}


//finds minimum adjacent cost given starting points, and order of directions to check
// is under is a bool input for being underground and returns if the best cost is still underground
int findCost(struct tile* map, int myBelt, int sizeX, int sizeY, struct toCheck startingPoint, int* whichX, int* whichY, bool* isUnder) {
  // direction order is VERY important! it must be consistent
  // first directino checked is the current direction (but we are looking backwards
  int directions[] = {(startingPoint.direction + 2) %4, NORTH, EAST, SOUTH, WEST};
  int numDirections = 5;
  // underground belts can't turn! (And the first point on a traceback already knows the direction)
  if (startingPoint.underground || startingPoint.goingDown || startingPoint.goingUp) {
    numDirections = 1;
  }
  int cost = INT_MAX;
  int startingPointCost = calcCost(map, sizeX, sizeY, myBelt, startingPoint) - startingPoint.cost;
  *whichX = startingPoint.x;
  *whichY = startingPoint.y;
  *isUnder = false;
  // go through each direction in the order given
  for (int i =0; i < numDirections; i++) {
    int direction = directions[i];
    // Don't allow going back on your self
    if (direction != startingPoint.direction) {
      int newX, newY;
      translate(startingPoint.x, startingPoint.y, direction, &newX, &newY);
      if (validPoint(newX, newY, sizeX, sizeY)) {
        struct tile thisTile = tileAt(map, sizeX, sizeY, newX, newY);
        // add cost if there is another belt endpoint nearby
        int thisCost = thisTile.cost;
        bool thisIsUnder = false;
        if (thisTile.hasBelt || thisTile.obstacle) {
          thisIsUnder = true;
        } else {
          thisIsUnder = false;
        }
        if (thisCost < cost && thisCost != 0 ) { 
          if((thisCost + startingPointCost) == startingPoint.cost) {

          cost = thisCost;
          *whichX = newX;
          *whichY = newY;
          *isUnder = thisIsUnder;
          } else if ((thisTile.underCost + startingPointCost) == startingPoint.cost){
          cost = thisTile.underCost;
          *whichX = newX;
          *whichY = newY;
          *isUnder = thisIsUnder;
            //printf("thisCost: %d, startingPiontCost: %d, cost: %d\n", thisCost, startingPointCost, startingPoint.cost);
          }
        }
      }
    }
   }
   return cost;
}
// check if a point is unoccupied/a fully valid point
// 0 for solution, 1 for empty, -1 for invalid
bool checkPoint(struct tile* map, int myBelt, int sizeX, int sizeY, struct toCheck currentPoint) {
  int startX = currentPoint.x;
  int startY = currentPoint.y;
  struct tile currentTile = map[startX + startY * sizeX];

  //off the edge of map, these are supposed to be detecting when adding options initially
  if (!validPoint(startX, startY, sizeX, sizeY)) {
    printf("WARNING: invalid point attempted: %d, %d\n", startX, startY);
    return false;
  } else {
    //(whichBelt is used to indicate already searched)
    if ((!currentPoint.underground) &&
          (currentTile.hasBelt ||  //belt here
          (currentTile.beltEndpoint >= 0 && currentTile.beltEndpoint != myBelt) || //a different belt's endpoint
          currentTile.obstacle)) { //an obstacle
      //this tile is occupied already
      return false;
    }// else if (currentTile.whichBelt == myBelt && (!(currentPoint.goingDown || currentPoint.goingUp))) {
      //we have already checked this spot
      //return false;
    //}
  }
  //this is a free point
  return true;
}
// add all possible routes from the current point to the toCheckList
// The routes are checked for validity but not for lack of collissions
void addAllOptions(struct tile* map, std::vector<struct toCheck>* toCheckList, int sizeX, int sizeY, struct toCheck currentPoint) {
  int currentY = currentPoint.y;
  int currentX = currentPoint.x;
  if (currentPoint.goingUp || currentPoint.goingDown || currentPoint.underground) {
    int newX, newY;
    translate(currentX, currentY, currentPoint.direction, &newX, &newY);
    debug_print_routing("adding options Underground at: %d, %d, dir:%d to: %d, %d\n", currentX, currentY, currentPoint.direction, newX, newY);
    if (validPoint(newX, newY, sizeX, sizeY)) {
      if (currentPoint.goingUp) {
        // normal above ground ahead
        toCheckList->push_back({newX, newY, currentPoint.direction, false, false, false, currentPoint.cost});
                                                            //    undeground under up
      } else if (currentPoint.goingDown) {
        toCheckList->push_back({newX, newY, currentPoint.direction, true, false, false, currentPoint.cost});
      } else { // underground
        // going up above ground (consider this first)
        toCheckList->push_back({newX, newY, currentPoint.direction, false, false, true, currentPoint.cost});
        // stayingn underground
        toCheckList->push_back({newX, newY, currentPoint.direction, true, false, false, currentPoint.cost});
      }
    }

  }else {
    debug_print_routing("Adding options Aboveground at: %d, %d, dir:%d\n", currentX, currentY, currentPoint.direction);
    int directions[] = {NORTH, EAST, SOUTH, WEST};
    int reverseDirection = (currentPoint.direction + 2) % 4;
    // STARTING POINT edge case. We want to consider every direction from the starting point
    if (currentPoint.direction < 0)
      reverseDirection = -1;
    //printf("Adding all options for %d, %d direction: %d (don't add dir %d)\n", currentX, currentY, currentPoint.direction, reverseDirection);
    // Check all directions
    for (int i = 0; i < 4; i++) {
      // don't add a doubling-back belt (this is impossible!)
      if (directions[i] != reverseDirection) {
        // spot the new belt would go
        int newX, newY;
        translate(currentX, currentY, directions[i], &newX, &newY);
        // stay above ground
        if (validPoint(newX, newY, sizeX, sizeY)) {
          if (obstacleAhead(map, sizeX, sizeY, newX, newY, directions[i])) {
            toCheckList->push_back({newX, newY, directions[i], false, true, false, currentPoint.cost});
            //                                              underground under up
          }
          // stay above ground ( we should check this after underground as a rule)
          toCheckList->push_back({newX, newY, directions[i], false, false, false, currentPoint.cost});
          //                                              underground under up`
        }
      }
    }
  }
    
}

//*********************************************
// Routing Function
// routes a connection from start to end and returns cost
// Currently does a basic maze try every route strategy
int routeNet(struct tile* map, int myBelt, int sizeX, int sizeY, struct net net) {
  //reset map costs
  for (int i =0; i < sizeX * sizeY; i ++ ) {
    map[i].cost = INT_MAX;
    map[i].underCost = INT_MAX;
  }

  //list of tiles to visit starting at the beginning (add to end to make FIFO)
  std::vector<struct toCheck> toCheckList;

  // create starting point
  struct toCheck startingPoint = {net.startX, net.startY, -1, false, false, false, 1};
 
  //if the starting point is invalid!
  if (!checkPoint(map, myBelt, sizeX, sizeY, startingPoint)){
    printf("ERROR: routing started on invalid point!\n");
    return -1;
  } 
  // add all starting tiles to visit
  addAllOptions(map, &toCheckList, sizeX, sizeY, startingPoint);

  struct tile* startingTile = pointerToTileAt(map, sizeX, sizeY, startingPoint.x, startingPoint.y);
  startingTile->cost = 1;
  startingTile->whichBelt = myBelt;
    //(which direction the shortest route is)
  // TODO: move these temp variables
  int whichX, whichY;
  bool isUnder;
  struct toCheck finalPoint;
  finalPoint.cost = -1;
  // number of extra steps after finding solution to run (to find a more optimal solution)
  int numAdditionalLoops = 15;
  int additionalLoops = -1;
  // *************************************************
  // main Routing loop
  //
  while (!toCheckList.empty()) {
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
    struct toCheck currentPoint = toCheckList.front();
    toCheckList.erase(toCheckList.begin());
    
    //printf("Routing: %u, %u, under:%d, going:%d\n", currentX, currentY, currentPoint.underground, currentPoint.goingUnder);
    // if this is a valid point
    if (checkPoint(map, myBelt, sizeX, sizeY, currentPoint)){
      currentPoint.cost = calcCost(map, sizeX, sizeY, myBelt, currentPoint);
      if (currentPoint.cost == INT_MAX) {
        printf("ERROR: invalid cost at %i, %i\n", currentPoint.x, currentPoint.y);
        printCosts(map, sizeX, sizeY);

      }
      // this is our solution! (if it isn't underground) TODO: WHY weird going down edge case? (A: it's because we check going down first...
      if (currentPoint.x == net.endX && currentPoint.y == net.endY && (!currentPoint.underground) && !currentPoint.goingDown) {
        // only accept better solutions
        if (finalPoint.cost > currentPoint.cost || finalPoint.cost < 0) {
          //found solution!!
          finalPoint = currentPoint;
          debug_print_routing("Found solution of cost: %d goingUp: %d direction: %d\n", currentPoint.cost, currentPoint.goingUp, currentPoint.direction);
          //TODO: decide how to add this back in w/o breaking anything
          additionalLoops = numAdditionalLoops; 
          if (additionalLoops == 0) {
            break;
          } else {
            continue;
          }
        }
      }  

      struct tile* thisTile = pointerToTileAt(map, sizeX, sizeY, currentPoint.x, currentPoint.y);

      //TEMP: in order to allow more options, accept options with the same costs

      if (currentPoint.goingDown) {
        debug_print_routing("Routing goingDown at: %i, %i cost: %i\n", currentPoint.x, currentPoint.y, currentPoint.cost);
      } else if (currentPoint.underground) {
        debug_print_routing("Routing underground at: %i, %i cost: %i\n",currentPoint.x, currentPoint.y, currentPoint.cost);
      } else if (currentPoint.goingUp) {
        debug_print_routing("Routing goingUp at: %i, %i cost: %i  \n", currentPoint.x, currentPoint.y, currentPoint.cost);
      } else {
        debug_print_routing("Routing aboveground at: %i, %i cost: %i  \n", currentPoint.x, currentPoint.y, currentPoint.cost);
      }
      // add all possible ways to go from here
      if (currentPoint.goingDown) {
        if (currentPoint.cost <= thisTile->underCost) {
          thisTile->underCost = currentPoint.cost;
          addAllOptions(map, &toCheckList, sizeX, sizeY, currentPoint);
        }
      } else {
        if (currentPoint.cost <= thisTile->cost) {
          thisTile->cost = currentPoint.cost;
          addAllOptions(map, &toCheckList, sizeX, sizeY, currentPoint);
        }
      }
    }
  }

  //**********************************
  // Retrace the route and mark it on map
  //
  if (finalPoint.cost != -1) {
    struct toCheck currentPoint = finalPoint;
    struct toCheck lastPoint = currentPoint;
    // start checking one over from final point
    debug_print_routing("**** starting retrace at: %d, %d facing: %d\n", currentPoint.x, currentPoint.y, currentPoint.direction);
    //currentPoint.un

    bool isThisFirstPoint = true;
    if (currentPoint.goingUp) {
      currentPoint.goingUp = false; //TODO: Why is this edge case a thing?
      //currentPoint.underground = true;
    }
    
    while (true) {
      //update last point
      lastPoint = currentPoint;
      // Find the cost of a new point and update
      currentPoint.cost = findCost(map, myBelt, sizeX, sizeY, currentPoint, &whichX, &whichY, &isUnder);
      
      //reverse the direeciton when retracing
      //currentPoint.direction = (currentPoint.direction + 2) % 4;
      struct tile* thisTile = map + currentPoint.x + currentPoint.y * sizeX;
      
      if (isUnder  && !currentPoint.underground) {
        // GOING UP (Backwards)
        debug_print_routing("Retracing going up: %i, %i dir: %d cost:%i\n", currentPoint.x, currentPoint.y, currentPoint.direction, currentPoint.cost);
        //currentPoint.goingUp = true;
        currentPoint.underground = true;
        thisTile->underground = true;
        thisTile->hasBelt = true;
        thisTile->whichBelt = myBelt;
        thisTile->beltDirection = currentPoint.direction;
      } else if (isUnder && (currentPoint.underground)) {
        // UNDER GROUND
        debug_print_routing("Retracing underground %i, %i dir: %d cost:%i\n", currentPoint.x, currentPoint.y, currentPoint.direction, currentPoint.cost);
        currentPoint.underground = true;
        //currentPoint.goingUp = false;
      } else if (!isUnder && currentPoint.underground) {
        currentPoint.underground = true;
        debug_print_routing("Retracing underground %i, %i dir: %d cost:%i\n", currentPoint.x, currentPoint.y, currentPoint.direction, currentPoint.cost);
        currentPoint.goingDown = true;
        currentPoint.underground = false;
      } else if (!isUnder && currentPoint.goingDown) {
        // GOING DOWN (backwards)
        debug_print_routing("Retracing going down %i, %i dir: %d cost:%i\n", currentPoint.x, currentPoint.y, currentPoint.direction, currentPoint.cost);
        currentPoint.underground = false;
        currentPoint.goingDown = false;
        thisTile->hasBelt = true;
        thisTile->underground = true;
        thisTile->beltDirection = currentPoint.direction;
        thisTile->whichBelt = myBelt;
      } else {
        // ABOVE GROUND
        debug_print_routing("Retracing above ground %i, %i dir: %d cost:%i\n", currentPoint.x, currentPoint.y, currentPoint.direction, currentPoint.cost);
        currentPoint.goingDown = false;
        thisTile->whichBelt = myBelt;
        thisTile->hasBelt = true;
        thisTile->beltDirection = currentPoint.direction;
      }
      if (currentPoint.x == net.startX && currentPoint.y == net.startY)
        break; // found solution!!
      // update to the next point
      currentPoint.x = whichX;
      currentPoint.y = whichY;

      if (lastPoint.y < currentPoint.y) // we are moving south (backwards)
        currentPoint.direction = NORTH;
      if (lastPoint.x > currentPoint.x) // we are moving west (backwards)
        currentPoint.direction = EAST;
      if (lastPoint.y > currentPoint.y) // we are moving north (backwards)
        currentPoint.direction = SOUTH;
      if (lastPoint.x < currentPoint.x) // we are moving east (backwards)
        currentPoint.direction = WEST;
  
      if (currentPoint.cost == INT_MAX || currentPoint.cost >= lastPoint.cost) {
        printf("ERROR! couldn't retrace route from %d, %d to %d, %d cost from %d to %d\n", lastPoint.x, lastPoint.y, currentPoint.x, currentPoint.y, lastPoint.cost, currentPoint.cost);
        printCosts(map, sizeX, sizeY);
        return -1;
      }
    }
    // we have to do this at the end so we don't mess up the traceback
    /*
    map[net.startX + (net.endY * sizeX)].whichBelt = myBelt;
    map[net.startX + (net.endY * sizeX)].hasBelt = true;
    map[net.startX + (net.endY * sizeX)].beltDirection = finalPoint.direction;
    if (finalPoint.goingUp) {
      map[net.endX + (net.endY * sizeX)].underground = true;
    }*/

    // TODO: in the future a final belt direction will need to be specified I think

  }
  return finalPoint.cost; //will be -1 if no route was found
}

// do all of the routing on a list of nets
// returns cost of all routes
// TODO: think about trying routing setups to find a better one?
// right now we quit after the first success
// Also, what should this return?
int route(struct tile* map, int sizeX, int sizeY, struct net* nets, int numNets) {
  debug_print_routing("***** Beginning Routing *****\n");
  int numAttempts = numNets; // must be less than numNets!!
  bool routed = false;
  int startingPoint = 0;
  int totalCost;
  //retry routings in different order until one works or we have tried them all
  while(!routed && numAttempts-- > 0) { 
    int i = startingPoint++;
    totalCost = 0;
    // clear map
    for (int i = 0; i < sizeX * sizeY; i++) {
      map[i].hasBelt = false;
      map[i].beltDirection = 0;
      map[i].whichBelt = -1;
      map[i].underground = false;
      map[i].cost = INT_MAX;
      map[i].underCost = INT_MAX;
      map[i].beltEndpoint = -1;
      //map[i].obstacle = false; //TEMP! move this so we can add obstacles before routing!
    }  
    // set belt enpoints on the map
    for (int i = 0; i < numNets; i++) {
      map[nets[i].startX + nets[i].startY * sizeX].beltEndpoint = i;
      map[nets[i].endX + nets[i].endY * sizeX].beltEndpoint = i;
    }
    // route each net
    for (int count = 0; count < numNets; count++) {
      debug_print_routing("routing net: %d\n", i);
      int cost = routeNet(map, i, sizeX, sizeY, nets[i]);
#ifdef DEBUG_ROUTING
      printMap(map, sizeX, sizeY);
      printCosts(map, sizeX, sizeY);
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
      debug_print_routing("Found successful routing order!\n");
      break;
    } else if(numAttempts > 0) {
      debug_print_routing("***** Trying new Routing order *******\n");
    }
  }
  if (!routed) {
    return -1;
  }
  return totalCost;
}

