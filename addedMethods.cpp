// In solveRouting() towards the top, replace reorderPins() with newReorderPins()

// Add the following structs to ece556.h
typedef struct {
    segment horizontalL1;
    segment verticalL1;
    segment horizontalL2;
    segment verticalL2;

    bool h1Valid;
    bool v1Valid;
    bool h2Valid;
    bool v2Valid;
} reorderSegment;

typedef struct {
    bool seen;
    int bestLIndex;
    point closestPointToL;
    int closestPointToLDistance;
} reorderBestSegment;

typedef struct {
    int numBetterIndices;
    int* listOfBetterIndex;
    point* listOfBetterPoint;
    int* listOfBetterDistance;
    long long totalDistanceSaved;
} reorderBetterL;


// Add the following code somewhere in the Cpp file
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
