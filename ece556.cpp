// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.h"
#include "printStuff.h"
#include <limits.h>
#include <string.h>

static int absInt(int value) {
    return (value < 0) ? -value : value;
}

static int isPointInGrid(routingInst *rst, point p) {
    if (rst == NULL) {
        return 0;
    }

    return (p.x >= 0 && p.x < rst->gx && p.y >= 0 && p.y < rst->gy);
}

static int computeStraightPathCost(routingInst *rst, point p1, point p2) {
    if (rst == NULL) {
        return INT_MAX;
    }

    // If the points are not aligned either horizontally or vertically, we cannot connect them with a straight segment, so return infinite cost.
    if (p1.x != p2.x && p1.y != p2.y) {
        return INT_MAX;
    }

    int x = p1.x;
    int y = p1.y;
    int dx = (p2.x > p1.x) ? 1 : ((p2.x < p1.x) ? -1 : 0);
    int dy = (p2.y > p1.y) ? 1 : ((p2.y < p1.y) ? -1 : 0);
    int cost = 0;

    // Iterate through each edge along the path and calculate the cost based on the projected utilization after adding this segment.
    while (x != p2.x || y != p2.y) {
        int nextX = x + dx;
        int nextY = y + dy;
        int edgeId = calculateEdgeNumber(rst, x, nextX, y, nextY);
        if (edgeId < 0 || edgeId >= rst->numEdges) {
            return INT_MAX;
        }

        int projectedUtil = rst->edgeUtils[edgeId] + 1;
        if (projectedUtil > rst->edgeCaps[edgeId]) {
            cost += projectedUtil - rst->edgeCaps[edgeId];
        }

        x = nextX;
        y = nextY;
    }

    return cost;
}

static int addStraightSegment(routingInst *rst, segment *seg, point p1, point p2) {
    if (rst == NULL || seg == NULL) {
        return 0;
    }

    // If the points are not aligned either horizontally or vertically, we cannot connect them with a straight segment, so return failure.
    if (p1.x != p2.x && p1.y != p2.y) {
        return 0;
    }

    seg->p1 = p1;
    seg->p2 = p2;
    seg->numEdges = absInt(p2.x - p1.x) + absInt(p2.y - p1.y);
    seg->edges = NULL;

    if (seg->numEdges == 0) {
        return 1;
    }

    seg->edges = (int *) malloc(seg->numEdges * sizeof(int));
    if (seg->edges == NULL) {
        return 0;
    }

    int x = p1.x;
    int y = p1.y;
    int dx = (p2.x > p1.x) ? 1 : ((p2.x < p1.x) ? -1 : 0);
    int dy = (p2.y > p1.y) ? 1 : ((p2.y < p1.y) ? -1 : 0);

    // Iterate through each edge along the path and add it to the segment's edge list, while also updating the edge utilization in the routing instance.
    for (int i = 0; i < seg->numEdges; i++) {
        int nextX = x + dx;
        int nextY = y + dy;
        int edgeId = calculateEdgeNumber(rst, x, nextX, y, nextY);
        if (edgeId < 0 || edgeId >= rst->numEdges) {
            free(seg->edges);
            seg->edges = NULL;
            seg->numEdges = 0;
            return 0;
        }

        seg->edges[i] = edgeId;
        rst->edgeUtils[edgeId]++;

        x = nextX;
        y = nextY;
    }

    return 1;
}

static int appendStraightSegment(routingInst *rst, route *nroute, point p1, point p2) {
    if (nroute == NULL) {
        return 0;
    }

    if (p1.x == p2.x && p1.y == p2.y) {
        return 1;
    }

    if (!addStraightSegment(rst, &(nroute->segments[nroute->numSegs]), p1, p2)) {
        return 0;
    }

    nroute->numSegs++;
    return 1;
}

static point chooseCorner(routingInst *rst, point start, point end) {
    point horizontalFirst;
    horizontalFirst.x = end.x;
    horizontalFirst.y = start.y;

    point verticalFirst;
    verticalFirst.x = start.x;
    verticalFirst.y = end.y;

    // Compute the cost of the two possible L-shaped routes (horizontal first vs vertical first) and choose the one with lower cost. If one of them is not feasible (i.e. has infinite cost), choose the other one. If both are not feasible, it doesn't matter which one we return since both will fail when we try to add the straight segments later.
    int horizontalFirstCost = computeStraightPathCost(rst, start, horizontalFirst);
    if (horizontalFirstCost != INT_MAX) {
        int secondLegCost = computeStraightPathCost(rst, horizontalFirst, end);
        if (secondLegCost == INT_MAX) {
            horizontalFirstCost = INT_MAX;
        } else {
            horizontalFirstCost += secondLegCost;
        }
    }

    // Similarly compute the cost for vertical first
    int verticalFirstCost = computeStraightPathCost(rst, start, verticalFirst);
    if (verticalFirstCost != INT_MAX) {
        int secondLegCost = computeStraightPathCost(rst, verticalFirst, end);
        if (secondLegCost == INT_MAX) {
            verticalFirstCost = INT_MAX;
        } else {
            verticalFirstCost += secondLegCost;
        }
    }

    if (verticalFirstCost < horizontalFirstCost) {
        return verticalFirst;
    }

    return horizontalFirst;
}

int readBenchmark(const char *fileName, routingInst *rst) {
    /*********** TO BE FILLED BY YOU **********/

    // This function relies on fgets(), which needs a buffer size to write to.
    // We are assuming each line will have a maximum of 30 characters.
    // If this assumption is wrong, change the size here.
    int BUFFER_LENGTH = 30 * sizeof(char);

    // This will be where we temporarily store each line
    // Since this is a .cpp file, malloc() needs to be casted to the appropriate type
    char* readFileBuffer = (char*) malloc(BUFFER_LENGTH);

    FILE* toRead = fopen(fileName, "r");

    if (readFileBuffer == NULL) {
        perror("Malloc Failed\n");
        return 0;
    }
        
    if (toRead == NULL) {
        perror("FILE issue\n");
        return 0;
    }

    // grid <gx> <gy>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    char* spaceDelimiter = strtok(readFileBuffer, " \t\n");

    // spaceDelimiter should just be "grid" at this point
    if (strcmp(spaceDelimiter, "grid")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    // Extract <gx> and <gy>
    rst->gx = strtol(strtok(NULL, " \t\n"), NULL, 10);
    rst->gy = strtol(strtok(NULL, " \t\n"), NULL, 10);

    // capacity <cap>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    spaceDelimiter = strtok(readFileBuffer, " \t\n");

    // spaceDelimiter should just be "capacity" at this point
    if (strcmp(spaceDelimiter, "capacity")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    rst->cap = strtol(strtok(NULL, " \t\n"), NULL, 10);
    
    // num net <numNets>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    spaceDelimiter = strtok(readFileBuffer, " \t\n");

    // spaceDelimiter should first be "num" at this point
    if (strcmp(spaceDelimiter, "num")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    // spaceDelimiter should now be "net"
    spaceDelimiter = strtok(NULL, " \t\n");
    if (strcmp(spaceDelimiter, "net")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    rst->numNets = strtol(strtok(NULL, " \t\n"), NULL, 10);

    rst->nets = (net*) malloc(rst->numNets * sizeof(net));

    for (int i = 0; i < rst->numNets; i++) {
        // n0 2
        // 0 2
        // 1 1
        // ...
        fgets(readFileBuffer, BUFFER_LENGTH, toRead);

        // Get the first part (n#)
        spaceDelimiter = strtok(readFileBuffer, " \t\n");

        // The first character will be "n" so we should ignore
        // Hence we start reading from index 1
        rst->nets[i].id = strtol(spaceDelimiter + 1, NULL, 10);

        rst->nets[i].numPins = strtol(strtok(NULL, " \t\n"), NULL, 10);

        rst->nets[i].pins = (point*) malloc(rst->nets[i].numPins * sizeof(point));

        for (int j = 0; j < rst->nets[i].numPins; j++) {
            // This would be each point: 0 2, 1 1, etc.
            fgets(readFileBuffer, BUFFER_LENGTH, toRead);
            spaceDelimiter = strtok(readFileBuffer, " \t\n");
            rst->nets[i].pins[j].x = strtol(spaceDelimiter, NULL, 10);
            rst->nets[i].pins[j].y = strtol(strtok(NULL, " \t\n"), NULL, 10);
        }

        rst->nets[i].nroute.numSegs = 0;
    }

    // Edge numbering follows what was recommended in the slides.
    // Start by numbering all horizontal edges (0-indexed)
    // Then do the first set of vertical edges (i.e. y=0 to y=1)
    // Then move up to (y=1 to y=2) and continue all the way

    rst->numEdges = rst->gy * (rst->gx - 1) + rst->gx * (rst->gy - 1);

    ////////// MODIFIED IN PART 2 /////////////////
    rst->edgeCaps = (int*) malloc(rst->numEdges * sizeof(int));
    rst->edgeUtils = (int*) malloc(rst->numEdges * sizeof(int));
    rst->edgeWeights = (int*) malloc(rst->numEdges * sizeof(int));
    rst->edgeHistory = (int*) malloc(rst->numEdges * sizeof(int));

    // Initialize all edges to the default capacity and all utilized edges to 0 since we don't use any
    for (int i = 0; i < rst->numEdges; i++) {
        rst->edgeCaps[i] = rst->cap;
        rst->edgeUtils[i] = 0;
        rst->edgeWeights[i] = 0;
        rst->edgeHistory[i] = 0;
    }

    //////////////////////////////////////////////

    // 2
    // 1 0 1 1 0
    // 2 2 2 1 0
    // etc...


    // 2
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);
    int numBlockages = strtol(readFileBuffer, NULL, 10);

    // 1 0 1 1 0
    for (int i = 0; i < numBlockages; i++) {
        fgets(readFileBuffer, BUFFER_LENGTH, toRead);

        int x_1 = strtol(strtok(readFileBuffer, " \t\n"), NULL, 10);
        int y_1 = strtol(strtok(NULL, " \t\n"), NULL, 10);
        int x_2 = strtol(strtok(NULL, " \t\n"), NULL, 10);
        int y_2 = strtol(strtok(NULL, " \t\n"), NULL, 10);
        int newSize = strtol(strtok(NULL, " \t\n"), NULL, 10);

        rst->edgeCaps[calculateEdgeNumber(rst, x_1, x_2, y_1, y_2)] = newSize;
    }

    free(readFileBuffer);
    fclose(toRead);

    return 1;
}

int solveRouting(routingInst *rst) {
    time_t begin;
    time(&begin);

    /*********** TO BE FILLED BY YOU **********/
    if (rst == NULL || rst->nets == NULL || rst->edgeCaps == NULL || rst->edgeUtils == NULL) {
        return 0;
    }

    // Initialize edgeUtils to 0 since we haven't routed anything yet
    for (int i = 0; i < rst->numEdges; i++) {
        rst->edgeUtils[i] = 0;
    }


    ////////// ADDED IN PART 2 /////////////////
    // printRoutingInst(rst, TRUE);
    reorderPins(rst);
    // printRoutingInst(rst, TRUE);
    ////////////////////////////////////////////

    // For each net, we will route in the order of the pins given.
    for (int i = 0; i < rst->numNets; i++) {

        net *currNet = &(rst->nets[i]);
        if (currNet->numPins < 1 || currNet->pins == NULL) {
            return 0;
        }

        currNet->nroute.numSegs = 0;
        currNet->nroute.segments = NULL;

        // The maximum number of segments we would need is 2 * (numPins - 1) since each pair of pins can require at most 2 segments (i.e. an L-shaped route).
        int maxNumSegs = 2 * (currNet->numPins - 1);
        if (maxNumSegs == 0) {
            continue;
        }

        currNet->nroute.segments = (segment *) malloc(maxNumSegs * sizeof(segment));
        if (currNet->nroute.segments == NULL) {
            return 0;
        }

        // Initialize all segments to have 0 edges and NULL edge arrays since we haven't added any segments yet
        for (int segIndex = 0; segIndex < maxNumSegs; segIndex++) {
            currNet->nroute.segments[segIndex].numEdges = 0;
            currNet->nroute.segments[segIndex].edges = NULL;
        }

        // Route in the order of the pins given. So if we have pins A, B, C, we will route A to B first and then B to C.
        for (int pinIndex = 1; pinIndex < currNet->numPins; pinIndex++) {
            point start = currNet->pins[pinIndex - 1];
            point end = currNet->pins[pinIndex];

            if (!isPointInGrid(rst, start) || !isPointInGrid(rst, end)) {
                return 0;
            }

            if (start.x == end.x || start.y == end.y) {
                if (!appendStraightSegment(rst, &(currNet->nroute), start, end)) {
                    return 0;
                }
                continue;
            }

            point corner = chooseCorner(rst, start, end);
            if (!appendStraightSegment(rst, &(currNet->nroute), start, corner)) {
                return 0;
            }
            if (!appendStraightSegment(rst, &(currNet->nroute), corner, end)) {
                return 0;
            }
        }

        if (currNet->nroute.numSegs == 0) {
            free(currNet->nroute.segments);
            currNet->nroute.segments = NULL;
        } else {
            segment *shrunkSegments = (segment *) realloc(
                currNet->nroute.segments,
                currNet->nroute.numSegs * sizeof(segment)
            );
            if (shrunkSegments != NULL) {
                currNet->nroute.segments = shrunkSegments;
            }
        }
    }

    // TODO: RRR, etc.
    for (int i = 0; i < rst->numEdges; i++) {
        updateEdgeWeights(rst, i);
    }

    int SECONDS_TO_WAIT = 20; /* 5 * 60; */ // Minimum amount of time was 5 minutes
    while (!checkTime(&begin, SECONDS_TO_WAIT)) {
        // Do this
        printf("WAITING...\n");
        // https://stackoverflow.com/questions/7684359/how-to-use-nanosleep-in-c-what-are-tim-tv-sec-and-tim-tv-nsec
        nanosleep((const struct timespec[]){{5, 0}}, NULL);
    }

    return 1;
}

int writeOutput(const char *outRouteFile, routingInst *rst) {
    /*********** TO BE FILLED BY YOU **********/
    FILE* toWrite = fopen(outRouteFile, "w");
    if (toWrite == NULL) {
        perror("Output file issue\n");
        return 0;
    }

    for (int i = 0; i < rst->numNets; i++) {
        fprintf(toWrite, "n%d\n", rst->nets[i].id);
        for (int j = 0; j < rst->nets[i].nroute.numSegs; j++) {
            fprintf(toWrite, "(%d,%d)-(%d,%d)\n", rst->nets[i].nroute.segments[j].p1.x,
                                                   rst->nets[i].nroute.segments[j].p1.y,
                                                   rst->nets[i].nroute.segments[j].p2.x,
                                                   rst->nets[i].nroute.segments[j].p2.y);
        }
        fprintf(toWrite, "!\n");
    }

    fclose(toWrite);
    return 1;
}

int release(routingInst *rst) {
    /*********** TO BE FILLED BY YOU **********/

    // Free "pins" in each net
    for (int i = 0; i < rst->numNets; i++) {
        free(rst->nets[i].pins);
        for (int j = 0; j < rst->nets[i].nroute.numSegs; j++) {
            // Free "edges" in each segment
            free(rst->nets[i].nroute.segments[j].edges);
        }

        // Free "segments" in each nroute
        free(rst->nets[i].nroute.segments);
    }

    // Free "nets"
    free(rst->nets);

    // Free "edge caps"
    free(rst->edgeCaps);

    // Free "edge utils"
    free(rst->edgeUtils);

    // ADDED IN PART 2 - free edgeWeights and edgeHistory
    free(rst->edgeWeights);
    free(rst->edgeHistory);


    return 1;
}


int calculateEdgeNumber(routingInst* rst, int x_1, int x_2, int y_1, int y_2) {
    if (x_1 > x_2) {
        // y must be equal, so swap the two so that x_1 < x_2
        int temp = x_1;
        x_1 = x_2;
        x_2 = temp;
    }
    if (y_1 > y_2) {
        // Similarly x must be equivalent so swap y-vals
        int temp = y_1;
        y_1 = y_2;
        y_2 = temp;
    }

    if (x_1 == x_2) {
        // Vertical Edge
        return rst->gy * (rst->gx - 1) + y_1 * rst->gx + x_1;
    }
    if (y_1 == y_2) {
        // Horizontal Edge
        return x_2 + y_1 * (rst->gx - 1) - 1;
    }

    // Ideally this would never run since the edge is either horizontal or vertical
    return -1;
}


void extraFunc() {
    /*
    point* p = (point*) malloc(1 * sizeof(point));
    p->x = 35;
    p->y = 23;
    printPoint(p, TRUE); */
}


/******************** ADDED PART 2 METHODS!!! ********************/

/**
 * Calculates distance between two points.
 * NOTE: manhattanDistance and rectilinear distance are the same thing.
 * Manhattan distance = |x_2 - x_1| + |y_2 - y_1|
 */
int manhattanDistance(point p1, point p2) {
    return absInt(p1.x - p2.x) + absInt(p1.y - p2.y);
}

/**
 * Finds the closest point to a given point.
 * Compares [startPoint, endPoint) to comparePoint
 * @param rst routingInstance pointer
 * @param netNum which net to use
 * @param comparePoint compare all other points to this one
 * @param startPoint index to start (inclusive)
 * @param endPoint index to end (EXclusive)
 */
int closestPoint(routingInst* rst, int netNum, int comparePoint, int startPoint, int endPoint) {
    int closestIndex = -1;
    int closestDistance = -1;
    for (int i = startPoint; i < endPoint; i++) {
        if (i == comparePoint) continue;
        int potentialDistance = manhattanDistance(rst->nets[netNum].pins[comparePoint], 
                                                rst->nets[netNum].pins[i]);
        if (closestDistance == -1 || potentialDistance < closestDistance) {
            closestIndex = i;
            closestDistance = potentialDistance;
        }
    }
    return closestIndex;
}

/**
 * Reorders each pin according to which next pin is the closest Manhattan distance away.
 * TODO: Rectilinear uses MBB to compare.
 *       However the only way I can think of doing that is to check all points on the MBB with all pin numbers.
 *       This would explode very quickly and I think in terms of time it may be better to do this.
 *       Otherwise we can code in MBB as well.
 */
void reorderPins(routingInst* rst) {

    // Each net is done separately
    for (int numNet = 0; numNet < rst->numNets; numNet++) {

        // The very last element doesn't need to be swapped (would be swapped with itself)
        // Thus we can save an iteration and only go up to numPins - 2
        for (int i = 0; i < rst->nets[numNet].numPins - 2; i++) {

            // swapIndex now has the best pin to follow the current pin with
            int swapIndex = closestPoint(rst, numNet, i, i + 1, rst->nets[numNet].numPins);

            // Swap both points
            point temp = rst->nets[numNet].pins[i + 1];
            rst->nets[numNet].pins[i + 1] = rst->nets[numNet].pins[swapIndex];
            rst->nets[numNet].pins[swapIndex] = temp;
        }
    }
}

/**
 * Updates a particular edge weight based on the formula used in the slides.
 * @param index 0-indexed edge to update weights for
 */
void updateEdgeWeights(routingInst* rst, int index) {
    int overflow_e = rst->edgeUtils[index] - rst->edgeCaps[index];

    // C does not have a max() function, so this changes it to 0 if 0 > u_e - c_e
    // Only if overflow_e is nonzero do we increment edgeHistory
    // Thus we can perform the check and implement edgesHistory at the same time
    if (overflow_e <= 0) overflow_e = 0;
    else rst->edgeHistory[index]++;

    rst->edgeWeights[index] = rst->edgeHistory[index] * overflow_e;
}

int checkTime(time_t* begin, int NUM_SECONDS) {
    // https://levelup.gitconnected.com/8-ways-to-measure-execution-time-in-c-c-48634458d0f9
    // #4 - using time()
    time_t end;
    time(&end);
    return (end - *begin > NUM_SECONDS) ? TRUE : FALSE; 
}
// ***************************************************************