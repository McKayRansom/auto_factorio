#include "stdio.h" // printf
#include "route.h"

#define SIZE_X 5
#define SIZE_Y 5

struct test {
  struct net nets[10];
  int numNets;
  char name[50];
  int cost;
};

struct test tests[] = {
  {{
    {0, 0, 4, 4}
  }, 1, "Basic", 9
},
  {{
    {4, 4, 0, 0}
  }, 1, "Reversed Basic", 9
},
{{
    {0, 0, 4, 4},
    {0, 4, 4, 0},
  }, 2, "2 routes", 18
},
{{
    {2, 0, 2, 4},
    {3, 0, 3, 4},
    {0, 2, 4, 2}
  }, 3, "long tunnel", 18
},
  {{
    {1, 0, 1, 4},
    {3, 0, 3, 4},
    {2, 0, 4, 2}
  }, 3, "can't tunnel", -1
},
  {{
    {0, 2, 4, 2},
    {4, 1, 3, 3},
  }, 2, "all the way around", 14
},
{{
    {0, 2, 4, 2},
    {0, 1, 1, 0},
    {0, 3, 4, 1},
  }, 3, "all the way around again", 20
},
{{
    {1, 0, 1, 4},
    {0, 2, 4, 2},
  }, 2, "starting tunnels", 11
},
{{
    {0, 1, 4, 1},
    {0, 3, 4, 3},
    {1, 0, 1, 4},
  }, 3, "tunnel gap", 11
}
};
int numTests = 9;

struct test transportBeltMadnessTests[] = {
  {{
    {3, 4, 13, 8}, //Iron plates
    {3, 5, 13, 7}, //copper plates
    {3, 7, 13, 4}, //Steel
    {3, 8, 13, 5}  //copper ore
  }, 4, "Part 1 - Warm Up"
},
{{
    {3, 2, 13, 7}, //Iron plates
    {3, 3, 13, 8}, //copper plates
    {3, 4, 13, 2}, //Steel
    {3, 5, 13, 9}, //copper ore

    {3, 7, 13, 3}, //Iron Ore
    {3, 8, 13, 10}, //Coal
    {3, 9, 13, 5}, //Wood
    {3, 10, 13, 4}  //Gear
  }, 8, "Part 2 - First Serious Head Scratching"
}

};
int numMadnessTests = 2;

struct point {
  int x;
  int y;
};
struct point obstacles[][50] = {{
// level 1
  {0,3}, //poles
  {0,9},
  {16,3},
  {16,9},
  {1,4}, //chest
  {1,5},
  {1,7},
  {1,8},
  {2,4}, //inserter
  {2,5},
  {2,7},
  {2,8},
  {15,4}, //chest (other side)
  {15,5},
  {15,7},
  {15,8},
  {14,4}, //inserter (other side)
  {14,5},
  {14,7},
  {14,8}},
// level 2
  {{0,3}, //poles
  {0,9},
  {16,3},
  {16,9},
  {1,2}, //chests top left
  {1,3},
  {1,4},
  {1,5},
  {1,7}, //chests bottom left
  {1,8},
  {1,9},
  {1,10},
  {15,2}, //chests top right
  {15,3},
  {15,4},
  {15,5},
  {15,7}, //chests bottom right
  {15,8},
  {15,9},
  {15,10},
  {2,2}, //inserters top left
  {2,3},
  {2,4},
  {2,5},
  {2,7}, //inserters bottom left
  {2,8},
  {2,9},
  {2,10},
  {14,2}, //inserters top right
  {14,3},
  {14,4},
  {14,5},
  {14,7}, //inserters bottom right
  {14,8},
  {14,9},
  {14,10},
  }
};
int numObstacles [] = {20, 36};


void testTransportBeltMadnessTests() {
  int sizeX = 17;
  int sizeY = 14;
  for (int i = 0; i < numMadnessTests; i++) {
    struct tile map[sizeX * sizeY];
    // add all obstacles to map
    for (int j = 0; j < sizeX * sizeY; j++) {
      map[j].obstacle = false;
    }
    for (int j = 0; j < numObstacles[i]; j++) {
      map[obstacles[i][j].x + sizeX * obstacles[i][j].y].obstacle = true;
    }
    printf("Transport belt madness: %d '%s'\n", i, transportBeltMadnessTests[i].name);
    //route the thing
    //int cost = route(map, sizeX, sizeY, transportBeltMadnessTests[i].nets, transportBeltMadnessTests[i].numNets, 0);
    if (cost < 0) {
      printf("FAILED TO ROUTE!!!\n");
    }
    printMap(map, sizeX, sizeY);
    printf("Total cost: %d\n", cost);
  }


}

void simpleTests() {
  bool failed = false;
  for (int i = 0; i < numTests; i++) {
    //struct tile* map = new struct tile[sizeX * sizeY];
    struct tile map[SIZE_X * SIZE_Y];
    for (int i = 0; i < SIZE_X * SIZE_Y; i++) {
      map[i].obstacle = false;
    } 
    printf("Test %d: %s\n", i, tests[i].name);
    struct netlist this_netlist;
    this_netlist.nets = test[i].nets;
    this_netlist.size = tist[i].numNets;
    int cost = route(map, this_netlist, 1);
    printMap(map, SIZE_X, SIZE_Y);
    if (cost != tests[i].cost) {
      printf("**************************************\n");
      printf("***   FAILED TEST %d : %s!!\n", i, tests[i].name);
      printf("***    cost: %d != %d\n", cost, tests[i].cost);
      printf("**************************************\n");
      failed = true;
      break;
    }
  }
  if (!failed) {
    printf("Routing PASSED %d tests\n", numTests);
  }
}

int main() {
  simpleTests();
  //testTransportBeltMadnessTests();
}

