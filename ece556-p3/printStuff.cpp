#include "printStuff.h"


void printPoint(point* p, int printNewLine) {
    if (p == NULL) printf("NULL");
    else printf("(%d, %d)", p->x, p->y);
    if (printNewLine == TRUE) printf("\n");
}


void printSegment(segment* s, int printNewLine) {
    if (s == NULL) printf("NULL");
    else {
        printf("{");
        printPoint(&(s->p1), FALSE);
        printf("->");
        printPoint(&(s->p2), FALSE);
        printf("}, edges: [");
        for (int i = 0; i < s->numEdges; i++) {
            printf("%d", s->edges[i]);
            if (i != s->numEdges - 1) printf(", ");
        }
        printf("]");
    }
    if (printNewLine == TRUE) printf("\n");
}


void printRoute(route* r, int printNewLine) {
    if (r == NULL) printf("NULL");
    else {
        printf("Route: [");
        for (int i = 0; i < r->numSegs; i++) {
            printf("{");
            printSegment(&(r->segments[i]), FALSE);
            printf("}");
            if (i != r->numSegs - 1) printf(", ");
        }
        printf("]");
        if (printNewLine == TRUE) printf("\n");
    }
}


void printNet(net* n, int printNewLine) {
    if (n == NULL) printf("NULL");
    else {
        printf("net: {id = %d, pins={", n->id);
        for (int i = 0; i < n->numPins; i++) {
            printPoint(&(n->pins[i]), FALSE);
            if (i != n->numPins - 1) printf(", ");
        }
        printf("}, route=");
        printRoute(&(n->nroute), FALSE);
        printf("}");
    }
    if (printNewLine == TRUE) printf("\n");
}


void printRoutingInst(routingInst* r, int printNewLine) {
    if (r == NULL) printf("NULL");
    else {
        printf("(%d, %d) capacity %d: ", r->gx, r->gy, r->cap);
        printf("nets: [");
        for (int i = 0; i < r->numNets; i++) {
            printNet(&(r->nets[i]), FALSE);
            if (i != r->numNets - 1) printf(", ");
        }
        printf("]");

        printf(", edgeCaps: [");
        for (int i = 0; i < r->numEdges; i++) {
            printf("%d", r->edgeCaps[i]);
            if (i != r->numEdges - 1) printf(", ");
        }

        printf("], edgeUtils: [");
        for (int i = 0; i < r->numEdges; i++) {
            printf("%d", r->edgeUtils[i]);
            if (i != r->numEdges - 1) printf(", ");
        }

        printf("]");
    }
    if (printNewLine == TRUE) printf("\n");
}