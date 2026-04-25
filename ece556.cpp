// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.

#include "ece556.h"
#include "printStuff.h"
#include <limits.h>
#include <queue>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vector>

void reorderPins(routingInst* rst);
void newReorderPins(routingInst* rst);
void updateEdgeWeights(routingInst* rst, int index);
int checkTime(time_t* begin, int NUM_SECONDS);

static int gUsePinOrdering = 0;
static int gUseNetOrderingAndRrr = 0;
static int gRrrIteration = 0;
static int gUseMinCostRouting = 0;

static void clearAllRoutes(routingInst *rst);
static int routeSingleNet(routingInst *rst, net *currNet);
static void recomputeEdgeWeight(routingInst *rst, int index, int updateHistory);
static void updateAllEdgeWeights(routingInst *rst, int updateHistory);
static void updateRouteEdgeWeights(routingInst *rst, route *nroute, int updateHistory);
static int findMinCostPathToRoute(routingInst *rst, point start, route *targetRoute, point **pathOut, int *pathLengthOut);
static int computeRrrTimeBudgetSeconds(routingInst *rst);
static int computeRerouteNetLimit(routingInst *rst);

void setRoutingMode(int usePinOrdering, int useNetOrderingAndRrr) {
    gUsePinOrdering = usePinOrdering ? 1 : 0;
    gUseNetOrderingAndRrr = useNetOrderingAndRrr ? 1 : 0;
}

static int absInt(int value) {
    return (value < 0) ? -value : value;
}

static int computeOverflowAmount(int utilization, int capacity) {
    int overflow = utilization - capacity;
    return (overflow > 0) ? overflow : 0;
}

static int computeRrrTimeBudgetSeconds(routingInst *rst) {
    (void) rst;

    // Part 2 expects the router to spend roughly five minutes total so the
    // RRR stage has time to make progress, while still leaving a small buffer
    // before a 300-second autograder cutoff. 
    // We can just replace this function with a constant if it ends up fixing the issue.
    return 290;
}

static int computeRerouteNetLimit(routingInst *rst) {
    if (rst == NULL || rst->numNets <= 0) {
        return 0;
    }

    long long gridPoints = (long long) rst->gx * (long long) rst->gy;
    int rerouteLimit = rst->numNets;

    // On very large benchmarks, spend the first few iterations on the most
    // congested nets so RRR can produce visible progress before the time budget
    // expires. Smaller benchmarks still reroute every net each pass.
    if (rst->numNets >= 300000 || gridPoints >= 500000) {
        if (gRrrIteration <= 1) {
            rerouteLimit = rst->numNets / 8;
        } else if (gRrrIteration == 2) {
            rerouteLimit = rst->numNets / 4;
        } else if (gRrrIteration == 3) {
            rerouteLimit = rst->numNets / 2;
        }
    } else if (rst->numNets >= 200000 || gridPoints >= 300000) {
        if (gRrrIteration <= 1) {
            rerouteLimit = rst->numNets / 4;
        } else if (gRrrIteration == 2) {
            rerouteLimit = rst->numNets / 2;
        }
    }

    if (rerouteLimit < 1) {
        rerouteLimit = 1;
    }
    if (rerouteLimit > rst->numNets) {
        rerouteLimit = rst->numNets;
    }

    return rerouteLimit;
}

static void recomputeEdgeWeight(routingInst *rst, int index, int updateHistory) {
    if (rst == NULL || index < 0 || index >= rst->numEdges) {
        return;
    }

    int overflow = computeOverflowAmount(rst->edgeUtils[index], rst->edgeCaps[index]);
    if (updateHistory && overflow > 0) {
        rst->edgeHistory[index]++;
    }

    rst->edgeWeights[index] = (overflow > 0) ? rst->edgeHistory[index] * overflow : 0;
}

static int isPointInGrid(routingInst *rst, point p) {
    if (rst == NULL) {
        return 0;
    }

    return (p.x >= 0 && p.x < rst->gx && p.y >= 0 && p.y < rst->gy);
}

static int pointToIndex(routingInst *rst, point p) {
    return p.y * rst->gx + p.x;
}

static point indexToPoint(routingInst *rst, int pointIndex) {
    point p;
    p.x = pointIndex % rst->gx;
    p.y = pointIndex / rst->gx;
    return p;
}

static int computeTraversalCost(routingInst *rst, point p1, point p2) {
    if (rst == NULL) {
        return INT_MAX;
    }

    int edgeId = calculateEdgeNumber(rst, p1.x, p2.x, p1.y, p2.y);
    if (edgeId < 0 || edgeId >= rst->numEdges) {
        return INT_MAX;
    }

    int projectedUtil = rst->edgeUtils[edgeId] + 1;
    int projectedOverflow = computeOverflowAmount(projectedUtil, rst->edgeCaps[edgeId]);
    int overflowPenalty = projectedOverflow * projectedOverflow;

    // Keep a unit wirelength term so zero-weight edges still prefer shorter paths.
    // Add both the historical edge weight and an immediate penalty for projected
    // overflow so rip-up/reroute can move off congested regions sooner.
    return 1 + rst->edgeWeights[edgeId] + overflowPenalty;
}

static int computeStraightPathCost(routingInst *rst, point p1, point p2) {
    if (rst == NULL) {
        return INT_MAX;
    }

    if (p1.x != p2.x && p1.y != p2.y) {
        return INT_MAX;
    }

    int x = p1.x;
    int y = p1.y;
    int dx = (p2.x > p1.x) ? 1 : ((p2.x < p1.x) ? -1 : 0);
    int dy = (p2.y > p1.y) ? 1 : ((p2.y < p1.y) ? -1 : 0);
    int cost = 0;

    while (x != p2.x || y != p2.y) {
        point currPoint;
        currPoint.x = x;
        currPoint.y = y;

        point nextPoint;
        nextPoint.x = x + dx;
        nextPoint.y = y + dy;

        int stepCost = computeTraversalCost(rst, currPoint, nextPoint);
        if (stepCost == INT_MAX || cost > INT_MAX - stepCost) {
            return INT_MAX;
        }

        cost += stepCost;
        x = nextPoint.x;
        y = nextPoint.y;
    }

    return cost;
}

static point chooseCorner(routingInst *rst, point start, point end) {
    point horizontalFirst;
    horizontalFirst.x = end.x;
    horizontalFirst.y = start.y;

    point verticalFirst;
    verticalFirst.x = start.x;
    verticalFirst.y = end.y;

    int horizontalFirstCost = computeStraightPathCost(rst, start, horizontalFirst);
    if (horizontalFirstCost != INT_MAX) {
        int secondLegCost = computeStraightPathCost(rst, horizontalFirst, end);
        if (secondLegCost == INT_MAX || horizontalFirstCost > INT_MAX - secondLegCost) {
            horizontalFirstCost = INT_MAX;
        } else {
            horizontalFirstCost += secondLegCost;
        }
    }

    int verticalFirstCost = computeStraightPathCost(rst, start, verticalFirst);
    if (verticalFirstCost != INT_MAX) {
        int secondLegCost = computeStraightPathCost(rst, verticalFirst, end);
        if (secondLegCost == INT_MAX || verticalFirstCost > INT_MAX - secondLegCost) {
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

static int ensureRouteCapacity(route *nroute, int *segmentCapacity, int neededCapacity) {
    if (nroute == NULL || segmentCapacity == NULL) {
        return 0;
    }

    if (*segmentCapacity >= neededCapacity) {
        return 1;
    }

    int newCapacity = (*segmentCapacity == 0) ? 4 : *segmentCapacity;
    while (newCapacity < neededCapacity) {
        newCapacity *= 2;
    }

    segment *newSegments = (segment *) realloc(nroute->segments, newCapacity * sizeof(segment));
    if (newSegments == NULL) {
        return 0;
    }

    for (int segIndex = *segmentCapacity; segIndex < newCapacity; segIndex++) {
        newSegments[segIndex].numEdges = 0;
        newSegments[segIndex].edges = NULL;
    }

    nroute->segments = newSegments;
    *segmentCapacity = newCapacity;
    return 1;
}

static int appendStraightSegment(routingInst *rst, route *nroute, int *segmentCapacity, point p1, point p2) {
    if (nroute == NULL || segmentCapacity == NULL) {
        return 0;
    }

    if (p1.x == p2.x && p1.y == p2.y) {
        return 1;
    }

    if (!ensureRouteCapacity(nroute, segmentCapacity, nroute->numSegs + 1)) {
        return 0;
    }

    if (!addStraightSegment(rst, &(nroute->segments[nroute->numSegs]), p1, p2)) {
        return 0;
    }

    nroute->numSegs++;
    return 1;
}

typedef struct {
    int pointIndex;
    int priority;
} searchState;

struct searchStateCompare {
    bool operator()(const searchState &left, const searchState &right) const {
        return left.priority > right.priority;
    }
};

static int findMinCostPathWithinBounds(
    routingInst *rst,
    point start,
    point end,
    point **pathOut,
    int *pathLengthOut,
    int minX,
    int maxX,
    int minY,
    int maxY
) {
    if (rst == NULL || pathOut == NULL || pathLengthOut == NULL) {
        return 0;
    }

    *pathOut = NULL;
    *pathLengthOut = 0;

    if (!isPointInGrid(rst, start) || !isPointInGrid(rst, end)) {
        return 0;
    }

    if (start.x == end.x && start.y == end.y) {
        *pathOut = (point *) malloc(sizeof(point));
        if (*pathOut == NULL) {
            return 0;
        }
        (*pathOut)[0] = start;
        *pathLengthOut = 1;
        return 1;
    }

    int numPoints = rst->gx * rst->gy;
    int startIndex = pointToIndex(rst, start);
    int endIndex = pointToIndex(rst, end);
    int *gCost = (int *) malloc(numPoints * sizeof(int));
    int *parent = (int *) malloc(numPoints * sizeof(int));
    char *closed = (char *) malloc(numPoints * sizeof(char));
    if (gCost == NULL || parent == NULL || closed == NULL) {
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    for (int pointIndex = 0; pointIndex < numPoints; pointIndex++) {
        gCost[pointIndex] = INT_MAX;
        parent[pointIndex] = -1;
        closed[pointIndex] = 0;
    }

    std::priority_queue<searchState, std::vector<searchState>, searchStateCompare> frontier;
    gCost[startIndex] = 0;
    searchState startState;
    startState.pointIndex = startIndex;
    startState.priority = absInt(start.x - end.x) + absInt(start.y - end.y);
    frontier.push(startState);

    while (!frontier.empty()) {
        searchState currState = frontier.top();
        frontier.pop();

        if (closed[currState.pointIndex]) {
            continue;
        }
        closed[currState.pointIndex] = 1;

        if (currState.pointIndex == endIndex) {
            break;
        }

        point currPoint = indexToPoint(rst, currState.pointIndex);
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int dir = 0; dir < 4; dir++) {
            point nextPoint;
            nextPoint.x = currPoint.x + dx[dir];
            nextPoint.y = currPoint.y + dy[dir];
            if (!isPointInGrid(rst, nextPoint)) {
                continue;
            }
            if (nextPoint.x < minX || nextPoint.x > maxX || nextPoint.y < minY || nextPoint.y > maxY) {
                continue;
            }

            int nextIndex = pointToIndex(rst, nextPoint);
            if (closed[nextIndex]) {
                continue;
            }

            int stepCost = computeTraversalCost(rst, currPoint, nextPoint);
            if (stepCost == INT_MAX || gCost[currState.pointIndex] > INT_MAX - stepCost) {
                continue;
            }

            int candidateCost = gCost[currState.pointIndex] + stepCost;
            if (candidateCost < gCost[nextIndex]) {
                gCost[nextIndex] = candidateCost;
                parent[nextIndex] = currState.pointIndex;

                int heuristic = absInt(nextPoint.x - end.x) + absInt(nextPoint.y - end.y);
                searchState nextState;
                nextState.pointIndex = nextIndex;
                nextState.priority = (candidateCost > INT_MAX - heuristic) ? INT_MAX : candidateCost + heuristic;
                frontier.push(nextState);
            }
        }
    }

    if (gCost[endIndex] == INT_MAX) {
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    int pathLength = 1;
    for (int pointIndex = endIndex; pointIndex != startIndex; pointIndex = parent[pointIndex]) {
        if (pointIndex < 0) {
            free(gCost);
            free(parent);
            free(closed);
            return 0;
        }
        pathLength++;
    }

    point *path = (point *) malloc(pathLength * sizeof(point));
    if (path == NULL) {
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    int writeIndex = pathLength - 1;
    for (int pointIndex = endIndex; writeIndex >= 0; pointIndex = parent[pointIndex]) {
        path[writeIndex] = indexToPoint(rst, pointIndex);
        if (pointIndex == startIndex) {
            break;
        }
        writeIndex--;
    }

    *pathOut = path;
    *pathLengthOut = pathLength;

    free(gCost);
    free(parent);
    free(closed);
    return 1;
}

static int findMinCostPath(routingInst *rst, point start, point end, point **pathOut, int *pathLengthOut) {
    if (rst == NULL) {
        return 0;
    }

    int fullMinX = 0;
    int fullMaxX = rst->gx - 1;
    int fullMinY = 0;
    int fullMaxY = rst->gy - 1;

    if (!gUseMinCostRouting) {
        return findMinCostPathWithinBounds(rst, start, end, pathOut, pathLengthOut, fullMinX, fullMaxX, fullMinY, fullMaxY);
    }

    int minX = (start.x < end.x) ? start.x : end.x;
    int maxX = (start.x > end.x) ? start.x : end.x;
    int minY = (start.y < end.y) ? start.y : end.y;
    int maxY = (start.y > end.y) ? start.y : end.y;
    int pinDistance = absInt(start.x - end.x) + absInt(start.y - end.y);
    int frameMargin = 8 + 4 * gRrrIteration + pinDistance / 8;
    if (rst->numNets >= 300000 || ((long long) rst->gx * (long long) rst->gy) >= 500000) {
        frameMargin = 4 + 3 * gRrrIteration + pinDistance / 16;
        if (frameMargin > 56) {
            frameMargin = 56;
        }
    } else if (frameMargin > 96) {
        frameMargin = 96;
    }

    minX -= frameMargin;
    minY -= frameMargin;
    maxX += frameMargin;
    maxY += frameMargin;

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= rst->gx) maxX = rst->gx - 1;
    if (maxY >= rst->gy) maxY = rst->gy - 1;

    if (findMinCostPathWithinBounds(rst, start, end, pathOut, pathLengthOut, minX, maxX, minY, maxY)) {
        return 1;
    }

    return findMinCostPathWithinBounds(rst, start, end, pathOut, pathLengthOut, fullMinX, fullMaxX, fullMinY, fullMaxY);
}

static void computeRouteBoundingBox(route *nroute, int *minX, int *maxX, int *minY, int *maxY) {
    if (nroute == NULL || nroute->numSegs <= 0 || minX == NULL || maxX == NULL || minY == NULL || maxY == NULL) {
        return;
    }

    *minX = nroute->segments[0].p1.x;
    *maxX = nroute->segments[0].p1.x;
    *minY = nroute->segments[0].p1.y;
    *maxY = nroute->segments[0].p1.y;

    for (int segIndex = 0; segIndex < nroute->numSegs; segIndex++) {
        segment *currSeg = &(nroute->segments[segIndex]);
        point segmentPoints[2] = {currSeg->p1, currSeg->p2};
        for (int pointIndex = 0; pointIndex < 2; pointIndex++) {
            point currPoint = segmentPoints[pointIndex];
            if (currPoint.x < *minX) *minX = currPoint.x;
            if (currPoint.x > *maxX) *maxX = currPoint.x;
            if (currPoint.y < *minY) *minY = currPoint.y;
            if (currPoint.y > *maxY) *maxY = currPoint.y;
        }
    }
}

static int populateRoutePointMask(routingInst *rst, route *nroute, char *routePointMask) {
    if (rst == NULL || nroute == NULL || routePointMask == NULL) {
        return 0;
    }

    for (int segIndex = 0; segIndex < nroute->numSegs; segIndex++) {
        segment *currSeg = &(nroute->segments[segIndex]);
        point currPoint = currSeg->p1;
        if (!isPointInGrid(rst, currPoint)) {
            return 0;
        }

        routePointMask[pointToIndex(rst, currPoint)] = 1;

        int dx = (currSeg->p2.x > currSeg->p1.x) ? 1 : ((currSeg->p2.x < currSeg->p1.x) ? -1 : 0);
        int dy = (currSeg->p2.y > currSeg->p1.y) ? 1 : ((currSeg->p2.y < currSeg->p1.y) ? -1 : 0);
        int numSteps = absInt(currSeg->p2.x - currSeg->p1.x) + absInt(currSeg->p2.y - currSeg->p1.y);
        for (int step = 0; step < numSteps; step++) {
            currPoint.x += dx;
            currPoint.y += dy;
            if (!isPointInGrid(rst, currPoint)) {
                return 0;
            }

            routePointMask[pointToIndex(rst, currPoint)] = 1;
        }
    }

    return 1;
}

static int distanceToBox(point p, int minX, int maxX, int minY, int maxY) {
    int dx = 0;
    int dy = 0;

    if (p.x < minX) {
        dx = minX - p.x;
    } else if (p.x > maxX) {
        dx = p.x - maxX;
    }

    if (p.y < minY) {
        dy = minY - p.y;
    } else if (p.y > maxY) {
        dy = p.y - maxY;
    }

    return dx + dy;
}

static int findMinCostPathToRouteWithinBounds(
    routingInst *rst,
    point start,
    route *targetRoute,
    point **pathOut,
    int *pathLengthOut,
    int minX,
    int maxX,
    int minY,
    int maxY
) {
    if (rst == NULL || targetRoute == NULL || pathOut == NULL || pathLengthOut == NULL) {
        return 0;
    }

    *pathOut = NULL;
    *pathLengthOut = 0;

    if (!isPointInGrid(rst, start) || targetRoute->numSegs <= 0 || targetRoute->segments == NULL) {
        return 0;
    }

    int numPoints = rst->gx * rst->gy;
    char *routePointMask = (char *) calloc(numPoints, sizeof(char));
    int *gCost = (int *) malloc(numPoints * sizeof(int));
    int *parent = (int *) malloc(numPoints * sizeof(int));
    char *closed = (char *) malloc(numPoints * sizeof(char));
    if (routePointMask == NULL || gCost == NULL || parent == NULL || closed == NULL) {
        free(routePointMask);
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    if (!populateRoutePointMask(rst, targetRoute, routePointMask)) {
        free(routePointMask);
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    int routeMinX = 0;
    int routeMaxX = 0;
    int routeMinY = 0;
    int routeMaxY = 0;
    computeRouteBoundingBox(targetRoute, &routeMinX, &routeMaxX, &routeMinY, &routeMaxY);

    int startIndex = pointToIndex(rst, start);
    if (routePointMask[startIndex]) {
        *pathOut = (point *) malloc(sizeof(point));
        if (*pathOut == NULL) {
            free(routePointMask);
            free(gCost);
            free(parent);
            free(closed);
            return 0;
        }

        (*pathOut)[0] = start;
        *pathLengthOut = 1;
        free(routePointMask);
        free(gCost);
        free(parent);
        free(closed);
        return 1;
    }

    for (int pointIndex = 0; pointIndex < numPoints; pointIndex++) {
        gCost[pointIndex] = INT_MAX;
        parent[pointIndex] = -1;
        closed[pointIndex] = 0;
    }

    std::priority_queue<searchState, std::vector<searchState>, searchStateCompare> frontier;
    gCost[startIndex] = 0;
    searchState startState;
    startState.pointIndex = startIndex;
    startState.priority = distanceToBox(start, routeMinX, routeMaxX, routeMinY, routeMaxY);
    frontier.push(startState);

    int foundIndex = -1;
    while (!frontier.empty()) {
        searchState currState = frontier.top();
        frontier.pop();

        if (closed[currState.pointIndex]) {
            continue;
        }
        closed[currState.pointIndex] = 1;

        if (routePointMask[currState.pointIndex]) {
            foundIndex = currState.pointIndex;
            break;
        }

        point currPoint = indexToPoint(rst, currState.pointIndex);
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int dir = 0; dir < 4; dir++) {
            point nextPoint;
            nextPoint.x = currPoint.x + dx[dir];
            nextPoint.y = currPoint.y + dy[dir];
            if (!isPointInGrid(rst, nextPoint)) {
                continue;
            }
            if (nextPoint.x < minX || nextPoint.x > maxX || nextPoint.y < minY || nextPoint.y > maxY) {
                continue;
            }

            int nextIndex = pointToIndex(rst, nextPoint);
            if (closed[nextIndex]) {
                continue;
            }

            int stepCost = computeTraversalCost(rst, currPoint, nextPoint);
            if (stepCost == INT_MAX || gCost[currState.pointIndex] > INT_MAX - stepCost) {
                continue;
            }

            int candidateCost = gCost[currState.pointIndex] + stepCost;
            if (candidateCost < gCost[nextIndex]) {
                gCost[nextIndex] = candidateCost;
                parent[nextIndex] = currState.pointIndex;

                int heuristic = distanceToBox(nextPoint, routeMinX, routeMaxX, routeMinY, routeMaxY);
                searchState nextState;
                nextState.pointIndex = nextIndex;
                nextState.priority = (candidateCost > INT_MAX - heuristic) ? INT_MAX : candidateCost + heuristic;
                frontier.push(nextState);
            }
        }
    }

    if (foundIndex < 0) {
        free(routePointMask);
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    int pathLength = 1;
    for (int pointIndex = foundIndex; pointIndex != startIndex; pointIndex = parent[pointIndex]) {
        if (pointIndex < 0) {
            free(routePointMask);
            free(gCost);
            free(parent);
            free(closed);
            return 0;
        }
        pathLength++;
    }

    point *path = (point *) malloc(pathLength * sizeof(point));
    if (path == NULL) {
        free(routePointMask);
        free(gCost);
        free(parent);
        free(closed);
        return 0;
    }

    int writeIndex = pathLength - 1;
    for (int pointIndex = foundIndex; writeIndex >= 0; pointIndex = parent[pointIndex]) {
        path[writeIndex] = indexToPoint(rst, pointIndex);
        if (pointIndex == startIndex) {
            break;
        }
        writeIndex--;
    }

    *pathOut = path;
    *pathLengthOut = pathLength;

    free(routePointMask);
    free(gCost);
    free(parent);
    free(closed);
    return 1;
}

static int findMinCostPathToRoute(routingInst *rst, point start, route *targetRoute, point **pathOut, int *pathLengthOut) {
    if (rst == NULL || targetRoute == NULL || targetRoute->numSegs <= 0 || targetRoute->segments == NULL) {
        return 0;
    }

    int fullMinX = 0;
    int fullMaxX = rst->gx - 1;
    int fullMinY = 0;
    int fullMaxY = rst->gy - 1;

    if (!gUseMinCostRouting) {
        return findMinCostPathToRouteWithinBounds(
            rst,
            start,
            targetRoute,
            pathOut,
            pathLengthOut,
            fullMinX,
            fullMaxX,
            fullMinY,
            fullMaxY
        );
    }

    int routeMinX = 0;
    int routeMaxX = 0;
    int routeMinY = 0;
    int routeMaxY = 0;
    computeRouteBoundingBox(targetRoute, &routeMinX, &routeMaxX, &routeMinY, &routeMaxY);

    int minX = (start.x < routeMinX) ? start.x : routeMinX;
    int maxX = (start.x > routeMaxX) ? start.x : routeMaxX;
    int minY = (start.y < routeMinY) ? start.y : routeMinY;
    int maxY = (start.y > routeMaxY) ? start.y : routeMaxY;
    int pinDistance = distanceToBox(start, routeMinX, routeMaxX, routeMinY, routeMaxY);
    int frameMargin = 8 + 4 * gRrrIteration + pinDistance / 8;
    if (rst->numNets >= 300000 || ((long long) rst->gx * (long long) rst->gy) >= 500000) {
        frameMargin = 4 + 3 * gRrrIteration + pinDistance / 16;
        if (frameMargin > 56) {
            frameMargin = 56;
        }
    } else if (frameMargin > 96) {
        frameMargin = 96;
    }

    minX -= frameMargin;
    minY -= frameMargin;
    maxX += frameMargin;
    maxY += frameMargin;

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= rst->gx) maxX = rst->gx - 1;
    if (maxY >= rst->gy) maxY = rst->gy - 1;

    if (findMinCostPathToRouteWithinBounds(rst, start, targetRoute, pathOut, pathLengthOut, minX, maxX, minY, maxY)) {
        return 1;
    }

    return findMinCostPathToRouteWithinBounds(
        rst,
        start,
        targetRoute,
        pathOut,
        pathLengthOut,
        fullMinX,
        fullMaxX,
        fullMinY,
        fullMaxY
    );
}

static int appendPathAsSegments(routingInst *rst, route *nroute, int *segmentCapacity, point *path, int pathLength) {
    // ADDED BY PRATEEK - this code is AI-generated
    if (rst == NULL || nroute == NULL || segmentCapacity == NULL || path == NULL || pathLength <= 0) {
        return 0;
    }

    // If the path is just a single point (meaning the pin is already on the route), 
    // no segments need to be drawn. Because the rest of the route is made of 
    // 1-unit segments, this point is guaranteed to be an explicit endpoint!
    if (pathLength == 1) {
        return 1;
    }

    // Instead of compressing straight lines, add a 1-unit segment for EVERY step.
    // This entirely eliminates T-junctions by making every grid coordinate an endpoint.
    for (int i = 0; i < pathLength - 1; i++) {
        if (!appendStraightSegment(rst, nroute, segmentCapacity, path[i], path[i + 1])) {
            return 0;
        }
    }

    return 1;
}

static void freeRoute(route *nroute) {
    if (nroute == NULL || nroute->segments == NULL) {
        return;
    }

    for (int segIndex = 0; segIndex < nroute->numSegs; segIndex++) {
        free(nroute->segments[segIndex].edges);
    }

    free(nroute->segments);
    nroute->segments = NULL;
    nroute->numSegs = 0;
}

static int applyRouteToEdgeUtils(routingInst *rst, route *nroute, int delta) {
    if (rst == NULL || nroute == NULL) {
        return 0;
    }

    for (int segIndex = 0; segIndex < nroute->numSegs; segIndex++) {
        segment *currSeg = &(nroute->segments[segIndex]);
        for (int edgeIndex = 0; edgeIndex < currSeg->numEdges; edgeIndex++) {
            int edgeId = currSeg->edges[edgeIndex];
            if (edgeId < 0 || edgeId >= rst->numEdges) {
                return 0;
            }

            rst->edgeUtils[edgeId] += delta;
            if (rst->edgeUtils[edgeId] < 0) {
                return 0;
            }
        }
    }

    return 1;
}

static int copyRoute(route *src, route *dst) {
    if (dst == NULL) {
        return 0;
    }

    dst->numSegs = 0;
    dst->segments = NULL;
    if (src == NULL || src->numSegs == 0 || src->segments == NULL) {
        return 1;
    }

    dst->segments = (segment *) malloc(src->numSegs * sizeof(segment));
    if (dst->segments == NULL) {
        return 0;
    }

    dst->numSegs = src->numSegs;
    for (int segIndex = 0; segIndex < src->numSegs; segIndex++) {
        dst->segments[segIndex].p1 = src->segments[segIndex].p1;
        dst->segments[segIndex].p2 = src->segments[segIndex].p2;
        dst->segments[segIndex].numEdges = src->segments[segIndex].numEdges;
        dst->segments[segIndex].edges = NULL;

        if (src->segments[segIndex].numEdges > 0) {
            dst->segments[segIndex].edges = (int *) malloc(src->segments[segIndex].numEdges * sizeof(int));
            if (dst->segments[segIndex].edges == NULL) {
                freeRoute(dst);
                return 0;
            }

            memcpy(
                dst->segments[segIndex].edges,
                src->segments[segIndex].edges,
                src->segments[segIndex].numEdges * sizeof(int)
            );
        }
    }

    return 1;
}

static void freeRouteArray(route *routes, int numRoutes) {
    if (routes == NULL) {
        return;
    }

    for (int routeIndex = 0; routeIndex < numRoutes; routeIndex++) {
        freeRoute(&(routes[routeIndex]));
    }

    free(routes);
}

static int copyAllRoutes(routingInst *rst, route **routesOut) {
    if (rst == NULL || routesOut == NULL) {
        return 0;
    }

    *routesOut = NULL;
    route *copiedRoutes = (route *) malloc(rst->numNets * sizeof(route));
    if (copiedRoutes == NULL) {
        return 0;
    }

    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        copiedRoutes[netIndex].numSegs = 0;
        copiedRoutes[netIndex].segments = NULL;
    }

    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        if (!copyRoute(&(rst->nets[netIndex].nroute), &(copiedRoutes[netIndex]))) {
            freeRouteArray(copiedRoutes, rst->numNets);
            return 0;
        }
    }

    *routesOut = copiedRoutes;
    return 1;
}

static int restoreAllRoutes(routingInst *rst, route *savedRoutes) {
    if (rst == NULL || savedRoutes == NULL) {
        return 0;
    }

    clearAllRoutes(rst);
    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        if (!copyRoute(&(savedRoutes[netIndex]), &(rst->nets[netIndex].nroute))) {
            clearAllRoutes(rst);
            return 0;
        }
    }

    return 1;
}

static int rebuildEdgeUtilsFromRoutes(routingInst *rst) {
    if (rst == NULL || rst->edgeUtils == NULL || rst->nets == NULL) {
        return 0;
    }

    for (int edgeIndex = 0; edgeIndex < rst->numEdges; edgeIndex++) {
        rst->edgeUtils[edgeIndex] = 0;
    }

    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        if (!applyRouteToEdgeUtils(rst, &(rst->nets[netIndex].nroute), 1)) {
            return 0;
        }
    }

    return 1;
}

static long long computeTotalWirelength(routingInst *rst) {
    if (rst == NULL || rst->nets == NULL) {
        return -1;
    }

    long long totalWirelength = 0;
    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        for (int segIndex = 0; segIndex < rst->nets[netIndex].nroute.numSegs; segIndex++) {
            totalWirelength += rst->nets[netIndex].nroute.segments[segIndex].numEdges;
        }
    }

    return totalWirelength;
}

static long long computeTotalOverflow(routingInst *rst) {
    if (rst == NULL || rst->edgeUtils == NULL || rst->edgeCaps == NULL) {
        return -1;
    }

    long long totalOverflow = 0;
    for (int edgeIndex = 0; edgeIndex < rst->numEdges; edgeIndex++) {
        int overflow = rst->edgeUtils[edgeIndex] - rst->edgeCaps[edgeIndex];
        if (overflow > 0) {
            totalOverflow += overflow;
        }
    }

    return totalOverflow;
}

static int rerouteNetsByOrdering(routingInst *rst, int *ordering) {
    if (rst == NULL || rst->nets == NULL) {
        return 0;
    }

    int rerouteLimit = computeRerouteNetLimit(rst);
    for (int orderedIndex = 0; orderedIndex < rerouteLimit; orderedIndex++) {
        int netIndex = (ordering == NULL) ? orderedIndex : ordering[orderedIndex];
        net *currNet = &(rst->nets[netIndex]);

        if (!applyRouteToEdgeUtils(rst, &(currNet->nroute), -1)) {
            return 0;
        }
        updateRouteEdgeWeights(rst, &(currNet->nroute), TRUE);

        freeRoute(&(currNet->nroute));
        if (!routeSingleNet(rst, currNet)) {
            return 0;
        }
        updateRouteEdgeWeights(rst, &(currNet->nroute), TRUE);
    }

    return 1;
}

static int routeSingleNet(routingInst *rst, net *currNet) {
    if (currNet == NULL || currNet->numPins < 1 || currNet->pins == NULL) {
        return 0;
    }

    freeRoute(&(currNet->nroute));
    currNet->nroute.numSegs = 0;
    currNet->nroute.segments = NULL;
    int segmentCapacity = 0;

    if (currNet->numPins == 1) {
        return 1;
    }

    // Route pins in order. During the RRR stage, connect each new pin to the
    // current route tree rather than forcing a simple pin-to-pin chain; this
    // gives the min-cost router freedom to build a lower-overflow tree.
    for (int pinIndex = 1; pinIndex < currNet->numPins; pinIndex++) {
        point start = currNet->pins[pinIndex - 1];
        point end = currNet->pins[pinIndex];

        if (!isPointInGrid(rst, start) || !isPointInGrid(rst, end)) {
            freeRoute(&(currNet->nroute));
            return 0;
        }

        // If this pin already lies on the current route tree, there is no need
        // to add a backtracking path just to visit it again.
        // if (currNet->nroute.numSegs > 0 && isPointOnRoute(&(currNet->nroute), end)) {
        //    continue;
        // }

        if (!gUseMinCostRouting) {
            if (start.x == end.x || start.y == end.y) {
                if (!appendStraightSegment(rst, &(currNet->nroute), &segmentCapacity, start, end)) {
                    freeRoute(&(currNet->nroute));
                    return 0;
                }
            } else {
                point corner = chooseCorner(rst, start, end);
                if (!appendStraightSegment(rst, &(currNet->nroute), &segmentCapacity, start, corner) ||
                    !appendStraightSegment(rst, &(currNet->nroute), &segmentCapacity, corner, end)) {
                    freeRoute(&(currNet->nroute));
                    return 0;
                }
            }
        } else if (currNet->nroute.numSegs == 0) {
            point *path = NULL;
            int pathLength = 0;
            if (!findMinCostPath(rst, start, end, &path, &pathLength)) {
                freeRoute(&(currNet->nroute));
                return 0;
            }

            int appendStatus = appendPathAsSegments(rst, &(currNet->nroute), &segmentCapacity, path, pathLength);
            free(path);
            if (!appendStatus) {
                freeRoute(&(currNet->nroute));
                return 0;
            }
        } else {
            point *path = NULL;
            int pathLength = 0;
            if (!findMinCostPathToRoute(rst, end, &(currNet->nroute), &path, &pathLength)) {
                freeRoute(&(currNet->nroute));
                return 0;
            }

            int appendStatus = appendPathAsSegments(rst, &(currNet->nroute), &segmentCapacity, path, pathLength);
            free(path);
            if (!appendStatus) {
                freeRoute(&(currNet->nroute));
                return 0;
            }
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

    return 1;
}

static void updateAllEdgeWeights(routingInst *rst, int updateHistory) {
    if (rst == NULL) {
        return;
    }

    for (int edgeIndex = 0; edgeIndex < rst->numEdges; edgeIndex++) {
        recomputeEdgeWeight(rst, edgeIndex, updateHistory);
    }
}

static void updateRouteEdgeWeights(routingInst *rst, route *nroute, int updateHistory) {
    if (rst == NULL || nroute == NULL || rst->numEdges <= 0) {
        return;
    }

    char *seenEdges = (char *) calloc(rst->numEdges, sizeof(char));
    if (seenEdges == NULL) {
        updateAllEdgeWeights(rst, updateHistory);
        return;
    }

    for (int segIndex = 0; segIndex < nroute->numSegs; segIndex++) {
        segment *currSeg = &(nroute->segments[segIndex]);
        for (int edgeIndex = 0; edgeIndex < currSeg->numEdges; edgeIndex++) {
            int edgeId = currSeg->edges[edgeIndex];
            if (edgeId < 0 || edgeId >= rst->numEdges || seenEdges[edgeId]) {
                continue;
            }

            seenEdges[edgeId] = 1;
            recomputeEdgeWeight(rst, edgeId, updateHistory);
        }
    }

    free(seenEdges);
}

static void clearAllRoutes(routingInst *rst) {
    if (rst == NULL || rst->nets == NULL) {
        return;
    }

    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        freeRoute(&(rst->nets[netIndex].nroute));
    }
}

static int computeNetRouteCost(routingInst *rst, net *currNet) {
    if (rst == NULL || currNet == NULL) {
        return 0;
    }

    int cost = 0;
    for (int segIndex = 0; segIndex < currNet->nroute.numSegs; segIndex++) {
        segment *currSeg = &(currNet->nroute.segments[segIndex]);
        for (int edgeIndex = 0; edgeIndex < currSeg->numEdges; edgeIndex++) {
            int edgeId = currSeg->edges[edgeIndex];
            if (edgeId >= 0 && edgeId < rst->numEdges) {
                cost += rst->edgeWeights[edgeId];
            }
        }
    }

    return cost;
}

typedef struct {
    int netIndex;
    int cost;
} netOrderingEntry;

static int compareNetOrderingEntries(const void *leftVoid, const void *rightVoid) {
    const netOrderingEntry *left = (const netOrderingEntry *) leftVoid;
    const netOrderingEntry *right = (const netOrderingEntry *) rightVoid;

    if (left->cost < right->cost) {
        return 1;
    }
    if (left->cost > right->cost) {
        return -1;
    }

    if (left->netIndex > right->netIndex) {
        return 1;
    }
    if (left->netIndex < right->netIndex) {
        return -1;
    }

    return 0;
}

static int *buildNetOrdering(routingInst *rst) {
    if (rst == NULL || rst->nets == NULL) {
        return NULL;
    }

    if (rst->numNets == 0) {
        return (int *) malloc(sizeof(int));
    }

    netOrderingEntry *entries = (netOrderingEntry *) malloc(rst->numNets * sizeof(netOrderingEntry));
    int *ordering = (int *) malloc(rst->numNets * sizeof(int));
    if (entries == NULL || ordering == NULL) {
        free(entries);
        free(ordering);
        return NULL;
    }

    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        entries[netIndex].netIndex = netIndex;
        entries[netIndex].cost = computeNetRouteCost(rst, &(rst->nets[netIndex]));
    }

    qsort(entries, rst->numNets, sizeof(netOrderingEntry), compareNetOrderingEntries);

    for (int netIndex = 0; netIndex < rst->numNets; netIndex++) {
        ordering[netIndex] = entries[netIndex].netIndex;
    }

    free(entries);
    return ordering;
}

static int routeNetsByOrdering(routingInst *rst, int *ordering) {
    if (rst == NULL || rst->nets == NULL) {
        return 0;
    }

    for (int orderedIndex = 0; orderedIndex < rst->numNets; orderedIndex++) {
        int netIndex = (ordering == NULL) ? orderedIndex : ordering[orderedIndex];
        if (!routeSingleNet(rst, &(rst->nets[netIndex]))) {
            clearAllRoutes(rst);
            return 0;
        }
    }

    return 1;
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
    // if (strcmp(spaceDelimiter, "grid")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    // Extract <gx> and <gy>
    rst->gx = strtol(strtok(NULL, " \t\n"), NULL, 10);
    rst->gy = strtol(strtok(NULL, " \t\n"), NULL, 10);

    // capacity <cap>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    spaceDelimiter = strtok(readFileBuffer, " \t\n");

    // spaceDelimiter should just be "capacity" at this point
    // if (strcmp(spaceDelimiter, "capacity")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    rst->cap = strtol(strtok(NULL, " \t\n"), NULL, 10);
    
    // num net <numNets>
    fgets(readFileBuffer, BUFFER_LENGTH, toRead);

    spaceDelimiter = strtok(readFileBuffer, " \t\n");

    // spaceDelimiter should first be "num" at this point
    // if (strcmp(spaceDelimiter, "num")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

    // spaceDelimiter should now be "net"
    spaceDelimiter = strtok(NULL, " \t\n");
    // if (strcmp(spaceDelimiter, "net")) printf("spaceDelimiter was \"%s\"\n", spaceDelimiter);

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
        rst->nets[i].nroute.segments = NULL;
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
        rst->edgeHistory[i] = 1;
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
    /*********** TO BE FILLED BY YOU **********/
    if (rst == NULL || rst->nets == NULL || rst->edgeCaps == NULL || rst->edgeUtils == NULL) {
        return 0;
    }

    time_t begin;
    time(&begin);

    // Initialize edgeUtils to 0 since we haven't routed anything yet
    for (int i = 0; i < rst->numEdges; i++) {
        rst->edgeUtils[i] = 0;
    }


    if (gUsePinOrdering) {
        // printf("Reordering...\n");
        newReorderPins(rst);
    }

    gRrrIteration = 0;
    gUseMinCostRouting = 0;

    if (!routeNetsByOrdering(rst, NULL)) {
        return 0;
    }

    if (!gUseNetOrderingAndRrr) {
        return 1;
    }

    updateAllEdgeWeights(rst, FALSE);

    route *bestRoutes = NULL;
    long long bestOverflow = computeTotalOverflow(rst);
    long long bestWirelength = computeTotalWirelength(rst);
    if (bestOverflow < 0 || bestWirelength < 0 || !copyAllRoutes(rst, &bestRoutes)) {
        clearAllRoutes(rst);
        return 0;
    }

    int SECONDS_TO_WAIT = computeRrrTimeBudgetSeconds(rst);
    while (bestOverflow > 0 && !checkTime(&begin, SECONDS_TO_WAIT)) {
        gRrrIteration++;
        gUseMinCostRouting = 1;

        // Apply net ordering from the part 2 slide deck:
        // cost(n) = sum of W_e over the stored route of n, then route high-cost nets first.
        int *netOrdering = buildNetOrdering(rst);
        if (netOrdering == NULL) {
            freeRouteArray(bestRoutes, rst->numNets);
            clearAllRoutes(rst);
            return 0;
        }

        if (!rerouteNetsByOrdering(rst, netOrdering)) {
            free(netOrdering);
            freeRouteArray(bestRoutes, rst->numNets);
            return 0;
        }

        free(netOrdering);

        long long currOverflow = computeTotalOverflow(rst);
        long long currWirelength = computeTotalWirelength(rst);
        if (currOverflow < bestOverflow ||
            (currOverflow == bestOverflow && currWirelength < bestWirelength)) {
            route *newBestRoutes = NULL;
            if (!copyAllRoutes(rst, &newBestRoutes)) {
                freeRouteArray(bestRoutes, rst->numNets);
                clearAllRoutes(rst);
                return 0;
            }

            freeRouteArray(bestRoutes, rst->numNets);
            bestRoutes = newBestRoutes;
            bestOverflow = currOverflow;
            bestWirelength = currWirelength;
        }
    }

    if (!restoreAllRoutes(rst, bestRoutes)) {
        freeRouteArray(bestRoutes, rst->numNets);
        return 0;
    }

    if (!rebuildEdgeUtilsFromRoutes(rst)) {
        freeRouteArray(bestRoutes, rst->numNets);
        clearAllRoutes(rst);
        return 0;
    }

    gUseMinCostRouting = 0;
    freeRouteArray(bestRoutes, rst->numNets);
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

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}
/**
 * 
 * @param p 
 * @param s 
 * @return int 
 */
point distanceFromSegment(point p, segment* s) {
    if (s == NULL) return p;

    // This was done previously but just to be safe we do it again here
    // (Shouldn't take that many extra checks/cycles)
    point s1, s2;
    s1.x = min(s->p1.x, s->p2.x);
    s1.y = min(s->p1.y, s->p2.y);
    s2.x = max(s->p1.x, s->p2.x);
    s2.y = max(s->p1.y, s->p2.y);

    // Return the point on this segment that is closest to this point
    if (s1.x == s2.x) {
        // This is a vertical segment

        // Check to see if the point is already on the segment
        if (s1.x == p.x && s1.y <= p.y && s2.y >= p.y) return p;

        // Check to see if the point's y-value is contained in the segment
        // If it is, return the point on this line
        if (s1.y <= p.y && s2.y >= p.y) return {s1.x, p.y};

        // Otherwise, return the bottommost point if p.y < s1.y or the topmost if p.y > s2.y
        return (p.y > s2.y) ? s2 : s1;
    }

    if (s1.y == s2.y) {
        // This is a vertical segment

        // Check to see if the point is already on the segment
        if (s1.y == p.y && s1.x <= p.x && s2.x >= p.x) return p;

        // Check to see if the point's x-value is contained in the segment
        // If it is, return the point on this line
        if (s1.x <= p.x && s2.x >= p.x) return {p.x, s1.y};

        // Otherwise, return the rightmost point if p.x < s1.x or the leftmost if p.x > s2.x
        return (p.x > s2.x) ? s2 : s1;
    }

    else return {-1, -1}; // Should never return
}

reorderSegment createReorderSegment(point p1, point p2) {
    reorderSegment theSegment;

    if (p1.x == p2.x) {
        // Straight vertical line
        theSegment.verticalL1.p1 = p1;
        theSegment.verticalL1.p2 = p2;
        theSegment.v1Valid = true;


        theSegment.h1Valid = false;
        theSegment.h2Valid = false;
        theSegment.v2Valid = false;
        return theSegment;
    }

    if (p1.y == p2.y) {
        // Straight horizontal line
        theSegment.horizontalL1.p1 = p1;
        theSegment.horizontalL1.p2 = p2;
        theSegment.horizontalL1.numEdges = 0;
        theSegment.h1Valid = true;


        theSegment.v1Valid = false;
        theSegment.h2Valid = false;
        theSegment.v2Valid = false;
        return theSegment;
    }

    // If we made it here, we will have an L shape
    // Organize so that s1 always has the lowest x value between p1 and p2
    // Note that p1.x and p2.x cannot equal each other since that was an if statement above
    point s1, s2;
    s1 = (p1.x < p2.x) ? p1 : p2;
    s2 = (p1.x < p2.x) ? p2 : p1;

    // Make both Ls: one that moves to the right then up/down
    theSegment.horizontalL1.p1 = s1;
    theSegment.horizontalL1.p2 = {s2.x, s1.y};
    theSegment.h1Valid = true;

    theSegment.verticalL1.p1 = {s2.x, s1.y};
    theSegment.verticalL1.p2 = s2;
    theSegment.v1Valid = true;

    // This L moves up/down then to the right
    theSegment.verticalL2.p1 = s1;
    theSegment.verticalL2.p2 = {s1.x, s2.y};
    theSegment.v2Valid = true;

    theSegment.horizontalL2.p1 = {s1.x, s2.y};
    theSegment.horizontalL2.p2 = s2;
    theSegment.h2Valid = true;

    return theSegment;
}

reorderSegment eliminateL(reorderBestSegment* bestSegmentList, int bestSegmentListSize, point* netPoints, 
                                    reorderSegment newSegment, int segmentNum, reorderBetterL l1, reorderBetterL l2) {
    segment allSegments[4] = {newSegment.horizontalL1, newSegment.verticalL1, newSegment.horizontalL2, newSegment.verticalL2};
    bool allValid[4] = {newSegment.h1Valid, newSegment.v1Valid, newSegment.h2Valid, newSegment.v2Valid};

    l1.numBetterIndices = 0;
    l1.totalDistanceSaved = 0;
    l2.numBetterIndices = 0;
    l2.totalDistanceSaved = 0;

    for (int i = 0; i < bestSegmentListSize; i++) {
        if (bestSegmentList[i].seen) continue;
        point currentPoint = netPoints[i];
        for (int j = 0; j < 4; j++) {
            if (!allValid[j]) continue;
            point currentClosest = distanceFromSegment(currentPoint, &allSegments[j]);
            int currentDistance = manhattanDistance(currentPoint, currentClosest);

            // Should closestPoint be INT_MAX, using long long allows the operation to not overflow
            // Otherwise, just two INT_MAXs could be enough to overflow
            long long currentImprovement = bestSegmentList[i].closestPointToLDistance - currentDistance;

            // Either this is an improvement, or this is the first go
            if (currentImprovement > 0) {
                if (j == 0 || j == 1) {
                    // First L
                    l1.listOfBetterIndex[l1.numBetterIndices] = i;
                    l1.totalDistanceSaved += currentImprovement;
                    l1.listOfBetterPoint[l1.numBetterIndices] = currentClosest;
                    l1.listOfBetterDistance[l1.numBetterIndices] = currentDistance;
                    l1.numBetterIndices++;
                }
                else {
                    // Second L
                    l2.listOfBetterIndex[l2.numBetterIndices] = i;
                    l2.totalDistanceSaved += currentImprovement;
                    l2.listOfBetterPoint[l2.numBetterIndices] = currentClosest;
                    l2.listOfBetterDistance[l2.numBetterIndices] = currentDistance;
                    l2.numBetterIndices++;
                }
            }
        }
    }

    // If decided later we can change the eval criteria to number of pins that improve.
    // Change second if statement below to if (l1.numBetterIndices >= l2.numBetterIndices) 

    // Checks which version has the best total amount of sharing and uses that
    if (l1.numBetterIndices == 0 && l2.numBetterIndices == 0) {
        // This segment is useless - don't even bother
        newSegment.h1Valid = false;
        newSegment.h2Valid = false;
        newSegment.v1Valid = false;
        newSegment.v2Valid = false;
        return newSegment;
    }
    if (l1.totalDistanceSaved >= l2.totalDistanceSaved) {
        newSegment.h2Valid = false;
        newSegment.v2Valid = false;
        for (int i = 0; i < l1.numBetterIndices; i++) {
            bestSegmentList[l1.listOfBetterIndex[i]].bestLIndex = segmentNum;
            bestSegmentList[l1.listOfBetterIndex[i]].closestPointToL = l1.listOfBetterPoint[i];
            bestSegmentList[l1.listOfBetterIndex[i]].closestPointToLDistance = l1.listOfBetterDistance[i];
        }
        return newSegment;
    }
    else {
        newSegment.h1Valid = false;
        newSegment.v1Valid = false;
        for (int i = 0; i < l2.numBetterIndices; i++) {
            bestSegmentList[l2.listOfBetterIndex[i]].bestLIndex = segmentNum;
            bestSegmentList[l2.listOfBetterIndex[i]].closestPointToL = l2.listOfBetterPoint[i];
            bestSegmentList[l2.listOfBetterIndex[i]].closestPointToLDistance = l2.listOfBetterDistance[i];
        }
        return newSegment;
    }

}

void newReorderPins(routingInst* rst) {
    // Each net is done separately
    for (int numNet = 0; numNet < rst->numNets; numNet++) {

        // Find the two closest points to each other
        // Start with the 0 index and whatever is closest to that
        if (rst->nets[numNet].numPins <= 2) continue;

        int closestIndex = closestPoint(rst, numNet, 0, 1, rst->nets[numNet].numPins);

        // This holds the top two closest points
        int closestPoints[2];

        // Initially we will have the very first point and whichever was closest to that
        closestPoints[0] = 0;
        closestPoints[1] = closestIndex;

        int closestDistance = manhattanDistance(rst->nets[numNet].pins[0], rst->nets[numNet].pins[closestIndex]);

        // Iterate through the remaining point and see if we can find a closer pair
        // If we can, update closestPoints and closestDistance
        for (int i = 1; i < rst->nets[numNet].numPins - 1; i++) {
            int temp = closestPoint(rst, numNet, i, i+1, rst->nets[numNet].numPins);
            if (manhattanDistance(rst->nets[numNet].pins[i], rst->nets[numNet].pins[temp]) < closestDistance) {
                closestPoints[0] = i;
                closestPoints[1] = temp;
                closestDistance = manhattanDistance(rst->nets[numNet].pins[i], rst->nets[numNet].pins[temp]);
            } 
        }
    
        // This is going to hold the new rst->nets[numNet].pins[] so we don't have to modify the original
        // The downside is we need to watch out for free() and malloc() as needed
        point* newNet = (point*) malloc(rst->nets[numNet].numPins * sizeof(point));
        newNet[0] = rst->nets[numNet].pins[closestPoints[0]];
        newNet[1] = rst->nets[numNet].pins[closestPoints[1]];
        int currentNewNetSize = 2;

        reorderBestSegment* bestSegmentList = (reorderBestSegment*) malloc(rst->nets[numNet].numPins * sizeof(reorderBestSegment));
        for (int i = 0; i < rst->nets[numNet].numPins; i++) {
            bestSegmentList[i].seen = false;
            bestSegmentList[i].bestLIndex = 0;
            bestSegmentList[i].closestPointToLDistance = INT_MAX;
        }

        // Instantiate a list of segments and add both L routes for the two points
        // This assumes two pins cannot be on the same square which I think is a fine assumption nto have
        std::vector<reorderSegment> segmentList;
        segmentList.reserve(2 * rst->nets[numNet].numPins);

        reorderBetterL* helperReorderL = (reorderBetterL*) malloc(2 * sizeof(reorderBetterL));
        for (int i = 0; i < 2; i++) {
            helperReorderL[i].listOfBetterDistance = (int*) malloc(2 * rst->nets[numNet].numPins * sizeof(int));
            helperReorderL[i].listOfBetterIndex = (int*) malloc(2 * rst->nets[numNet].numPins * sizeof(int));
            helperReorderL[i].listOfBetterPoint = (point*) malloc(2 * rst->nets[numNet].numPins * sizeof(point));
        }

        reorderSegment firstSegment = eliminateL(bestSegmentList, rst->nets[numNet].numPins, rst->nets[numNet].pins, 
            createReorderSegment(rst->nets[numNet].pins[closestPoints[0]], 
                    rst->nets[numNet].pins[closestPoints[1]]), 0, helperReorderL[0], helperReorderL[1]);
        
        segmentList.push_back(firstSegment);

        bestSegmentList[closestPoints[0]].seen = true;
        bestSegmentList[closestPoints[1]].seen = true;

        // Continue doing this until we run out of points
        while (currentNewNetSize != rst->nets[numNet].numPins) {

            // Find the point on the L path that is closest to this point
            int closestSegmentIndex = -1; // Holds which L reorderSegment we will use
            int closestPointIndex = -1; // Holds which point that we haven't fitted is the closest
            int closestLIndex = -1; // Holds the index of the particular horizontal/vertical segment we are using from reorderSegment
            point pointClosest; // Holds the closest point within the segment
            int pointClosestDistance = -1; // Holds smallest distance between pointClosest and the point we are looking at

            reorderBestSegment currentBest;

            for (int pointIndex = 0; pointIndex < rst->nets[numNet].numPins; pointIndex++) {

                if (bestSegmentList[pointIndex].seen) continue; // Do not repeat points

                if (pointClosestDistance == -1 || bestSegmentList[pointIndex].closestPointToLDistance < pointClosestDistance) {
                    closestPointIndex = pointIndex;
                    closestLIndex = bestSegmentList[pointIndex].bestLIndex;
                    pointClosest = bestSegmentList[pointIndex].closestPointToL;
                    pointClosestDistance = bestSegmentList[pointIndex].closestPointToLDistance;
                }
            }


            // Once we make it here, we have the closest segment and closest point - update the L segments and the lists
            if (pointClosest.x != rst->nets[numNet].pins[closestPointIndex].x || 
                pointClosest.y != rst->nets[numNet].pins[closestPointIndex].y) {
                // Only add a new segment if the closestPoint and the pin are NOT the same
                // (if they were, the segment would be size 0)

                reorderSegment temp = eliminateL(bestSegmentList, rst->nets[numNet].numPins, rst->nets[numNet].pins, 
                                                createReorderSegment(pointClosest, 
                                                        rst->nets[numNet].pins[closestPointIndex]), segmentList.size(), helperReorderL[0], helperReorderL[1]);
                if (temp.h1Valid || temp.h2Valid || temp.v1Valid || temp.v2Valid) segmentList.push_back(temp);
            }

            // Update newNet and closestPoints
            newNet[currentNewNetSize] = rst->nets[numNet].pins[closestPointIndex];
            bestSegmentList[closestPointIndex].seen = true;
            currentNewNetSize++;
        }

        // We are done - free old list and use the new list we created.
        free(rst->nets[numNet].pins);
        for (int i = 0; i < 2; i++) {
            free(helperReorderL[i].listOfBetterDistance);
            free(helperReorderL[i].listOfBetterIndex);
            free(helperReorderL[i].listOfBetterPoint);
        }
        free(helperReorderL);
        free(bestSegmentList);
        rst->nets[numNet].pins = newNet;
    }
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
    recomputeEdgeWeight(rst, index, TRUE);
}

int checkTime(time_t* begin, int NUM_SECONDS) {
    // https://levelup.gitconnected.com/8-ways-to-measure-execution-time-in-c-c-48634458d0f9
    // #4 - using time()
    time_t end;
    time(&end);
    return (end - *begin > NUM_SECONDS) ? TRUE : FALSE; 
}
// ***************************************************************
