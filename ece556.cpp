// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.h"
#include "printStuff.h"
#include <string.h>

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

    char* spaceDelimiter = strtok(readFileBuffer, " ");

    // spaceDelimiter should just be "grid" at this point
    if (strcmp(spaceDelimiter, "grid")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    // Extract <gx> and <gy>
    rst->gx = strtol(strtok(NULL, " "), NULL, 10);
    rst->gy = strtol(strtok(NULL, " "), NULL, 10);

    // capacity <cap>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    spaceDelimiter = strtok(readFileBuffer, " ");

    // spaceDelimiter should just be "capacity" at this point
    if (strcmp(spaceDelimiter, "capacity")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    rst->cap = strtol(strtok(NULL, " "), NULL, 10);
    
    // num net <numNets>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    spaceDelimiter = strtok(readFileBuffer, " ");

    // spaceDelimiter should first be "num" at this point
    if (strcmp(spaceDelimiter, "num")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    // spaceDelimiter should now be "net"
    spaceDelimiter = strtok(NULL, " ");
    if (strcmp(spaceDelimiter, "net")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    rst->numNets = strtol(strtok(NULL, " "), NULL, 10);

    rst->nets = (net*) malloc(rst->numNets * sizeof(net));

    for (int i = 0; i < rst->numNets; i++) {
        // n0 2
        // 0 2
        // 1 1
        // ...
        fgets(readFileBuffer, BUFFER_LENGTH, toRead);

        // Get the first part (n#)
        spaceDelimiter = strtok(readFileBuffer, " ");

        // The first character will be "n" so we should ignore
        // Hence we start reading from index 1
        rst->nets[i].id = strtol(spaceDelimiter + 1, NULL, 10);

        rst->nets[i].numPins = strtol(strtok(NULL, " "), NULL, 10);

        rst->nets[i].pins = (point*) malloc(rst->nets[i].numPins * sizeof(point));

        for (int j = 0; j < rst->nets[i].numPins; j++) {
            // This would be each point: 0 2, 1 1, etc.
            fgets(readFileBuffer, BUFFER_LENGTH, toRead);
            spaceDelimiter = strtok(readFileBuffer, " ");
            rst->nets[i].pins[j].x = strtol(spaceDelimiter, NULL, 10);
            rst->nets[i].pins[j].y = strtol(strtok(NULL, " "), NULL, 10);
        }

        rst->nets[i].nroute.numSegs = 0;
    }

    // Edge numbering follows what was recommended in the slides.
    // Start by numbering all horizontal edges (0-indexed)
    // Then do the first set of vertical edges (i.e. y=0 to y=1)
    // Then move up to (y=1 to y=2) and continue all the way

    rst->numEdges = rst->gy * (rst->gx - 1) + rst->gx * (rst->gy - 1);

    rst->edgeCaps = (int*) malloc(rst->numEdges * sizeof(int));
    rst->edgeUtils = (int*) malloc(rst->numEdges * sizeof(int));

    // Initialize all edges to 1 and all utilized edges to 0 since we don't use any
    for (int i = 0; i < rst->numEdges; i++) {
        rst->edgeCaps[i] = 1;
        rst->edgeUtils[i] = 0;
    }

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

        int x_1 = strtol(strtok(readFileBuffer, " "), NULL, 10);
        int y_1 = strtol(strtok(NULL, " "), NULL, 10);
        int x_2 = strtol(strtok(NULL, " "), NULL, 10);
        int y_2 = strtol(strtok(NULL, " "), NULL, 10);
        int newSize = strtol(strtok(NULL, " "), NULL, 10);

        rst->edgeCaps[calculateEdgeNumber(rst, x_1, x_2, y_1, y_2)] = newSize;
    }

    free(readFileBuffer);
    fclose(toRead);

    return 1;
}

int solveRouting(routingInst *rst) {
    /*********** TO BE FILLED BY YOU **********/


    // TODO: ACTUALLY IMPLEMENT THIS
    // THIS IS JUST THERE TO TEST writeOutput()

    rst->nets[0].nroute.numSegs = 2;
    rst->nets[0].nroute.segments = (segment*) malloc(rst->nets[0].nroute.numSegs * sizeof(segment));
    rst->nets[0].nroute.segments[0].numEdges = 3;
    rst->nets[0].nroute.segments[0].edges = (int*) malloc(rst->nets[0].nroute.segments[0].numEdges * sizeof(int));
    rst->nets[0].nroute.segments[0].edges[0] = 2;
    rst->nets[0].nroute.segments[0].edges[1] = 3;
    rst->nets[0].nroute.segments[0].edges[2] = 4;

    rst->nets[0].nroute.segments[0].p1.x = 5;
    rst->nets[0].nroute.segments[0].p1.y = 2;
    rst->nets[0].nroute.segments[0].p2.x = 3;
    rst->nets[0].nroute.segments[0].p2.y = 8;


    rst->nets[0].nroute.segments[1].numEdges = 2;
    rst->nets[0].nroute.segments[1].edges = (int*) malloc(rst->nets[0].nroute.segments[0].numEdges * sizeof(int));
    rst->nets[0].nroute.segments[1].edges[0] = 10;
    rst->nets[0].nroute.segments[1].edges[1] = 20;

    rst->nets[0].nroute.segments[1].p1.x = 1;
    rst->nets[0].nroute.segments[1].p1.y = 68;
    rst->nets[0].nroute.segments[1].p2.x = 226;
    rst->nets[0].nroute.segments[1].p2.y = 65;

    rst->nets[1].nroute.numSegs = 1;
    rst->nets[1].nroute.segments = (segment*) malloc(rst->nets[1].nroute.numSegs * sizeof(segment));
    rst->nets[1].nroute.segments[0].numEdges = 1;
    rst->nets[1].nroute.segments[0].edges = (int*) malloc(rst->nets[1].nroute.segments[0].numEdges * sizeof(int));
    rst->nets[1].nroute.segments[0].edges[0] = 2222;

    rst->nets[1].nroute.segments[0].p1.x = 51;
    rst->nets[1].nroute.segments[0].p1.y = 52;
    rst->nets[1].nroute.segments[0].p2.x = 53;
    rst->nets[1].nroute.segments[0].p2.y = 54;

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
            fprintf(toWrite, "(%d, %d) - (%d, %d)\n", rst->nets[i].nroute.segments[j].p1.x,
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
