# Part 3 Final Report - Prateek Tandon

## Improvement
I had previously made the pin ordering function in Part 2. For part 3, I decided to take this function and update it to use a better heuristic.

## Opportunity for Improvement

To keep it simple, my pin ordering function in Part 2 took the 0th point provided as the first point and simply found the closest point to the previous point using Manhattan distance. Whichever point was closest to the previous point would be moved up in the order and the process would repeat. Rather than using this, I wanted to use the Steiner algorithm we had discussed in class in the hopes that when fed into RRR a better pin ordering would further reduce time.

## Procedure

I mainly took the pseudocode provided in lecture and attempted to implement it. When beginning, the hardest part was figuring out how to find the pin closest to points on the MBB - I could have compared all points on the line but that is slow and often redundant. 

I realized that there are a few main points that I need to examine in order to find the closest point, namely:

- If the pin happened to be on the MBB to begin with, that is obviously the closest point and has a distance of 0.

- If this was a horizontal line and the pin happened to be within the y span (i.e. there was a point on the line directly above or below the pin without moving in the x direction), that will be the closest point. This is only true because we were using the Manhattan distance.

- Repeat the above logic if this was a vertical line

- Any pins that have not been dealt with yet are either completely to the left or completely to the right of the line. Thus the closest point will be the leftmost point or the rightmost point. This is similar for if the line was vertical.

By following the above steps, we only have to check the range of the line and do not need to check each individual point. Thus this algorithm is O(1) compared to the size of the line.

Once I had that, I worked to make a custom "L" data structure for my method. 

Finally, I wrote the method. It originally involved moving through all points to see if we had visited them, and seeing which point was now closest to an L segment. This point would be selected and BOTH L segments would be added. Once we aligned to an L segment, we make the other L segment invalid (there are two L segments in an MBB - remove whichever wasn't used) and continued in this manner. 

Once I had the method, I worked to improve the runtime by calculating the distance as soon as I inserted the L rather than on every iteration so I didn't have to repeatedly perform distance calculations that I had previously done before. I also improved checking for already visited points from a single list to an index-based list so that checks could be O(1) checking instead of O(N). 


## Evaluation

| Design | Part 2 Submission |  |  | Part 3 Individual Submission  |  |  | 
| --- | ---: | ---: | ---: | ---: | ---: | ---: | 
|  | RUN | TOF | Q | RUN | TOF | Q |
| adaptec1 | 5m30s | 151475 | 207015.83 | 5m39s | 147089 | 202492.5233 |
| adaptec2 | 5m39s | 716 | 985.69 | 5m43s | 770 | 1063.5 |
| adaptec3 | 7m47s | 86208 | 130940.4 | 7m56s | 85496 | 130713.9 |

I have noticed that sometimes the new pin ordering actually is worse than the old algorithm. I believe this is just because no routing is done with this, it can only potentially improve RRR. This also is not the most optimal way to order the pins as that problem is NP-Complete. On most cases it does seem to help but sometimes I guess the generated order actually hurts the logic.