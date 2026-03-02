#ifndef PRINT_HELPER_H
#define PRINT_HELPER_H

// This class just prints stuff so we can see output in console.
// Hopefully this will help more than the debugger.
// The second parameter is whether you want \n printed at the end.
// Pass in TRUE (or 1) to print \n

#include "ece556.h"

#define TRUE 1
#define FALSE 0

// Prints (x, y)
void printPoint(point* p, int printNewLine);

// Prints {(p1_x, p1_y) -> (p2_x, p2_y)}, edges: [***ALL EDGES***]
void printSegment(segment* s, int printNewLine);

// Prints Route: {[*SEGMENT LIST**]}
void printRoute(route* r, int printNewLine);

// Prints net: {id = net_id, points = {list of points}, route=***ROUTE***}
void printNet(net* n, int printNewLine);

// Prints (gx, gy)
void printRoutingInst(routingInst* r, int printNewLine);

#endif