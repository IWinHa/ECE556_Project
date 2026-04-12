```
1
```
## ECE556 Project:

## Part 2

#### Azadeh Davoodi


## Outline

- Routing framework
- Suggestion for teamwork
- Minimum project requirements
- Format of executable and required input

### arguments

- What to submit?
- Project grading

```
2
```

## Routing Framework

- Components for project
    1. Pin ordering
    2. Generate initial solution and assign
       initial edge weights
    3. Rip-up and reroute
       3.1 “Net” ordering
       3.2 RRR each net based on min-
       cost routing; update edge
       weights at each step
    4. Termination condition

```
3
```
```
Generate Initial Solution
```
```
Rip-up and re-route
( RRR )
```
```
Terminate?
```
```
No
```
```
Pin ordering
```
```
Yes
```

```
1
2
```
```
3
```
```
4
6 5
7
```
```
1
2
```
(^31)
2
3
4
5
1
2
3
4
6 5
7
1
2
3
4
1
2
3
4
6 5

# 1 2 3

# 4 5 6

## (1) Pin Ordering

- Order the pins of a net
    - Assuming a 2-pin routing procedure
       which connects the pins of a net in
       that order
    - This step is reordering of the entries
       in the pins array of the net data
       structure
- You can adapt a simpler variation
    of the heuristic for rectilinear
    routing for pin ordering
       - Slide 10 in the W7-GlobalRouting-
          SingleNetRouting slide deck
- Details of your implementation
    does not matter as long as
    evaluation criteria is passed


## (2) Generate Initial Solution

- This part was already done in the first part of the project

#### with the caveat that

- now pin ordering is initially applied so your generated initial
    solution will be different than part 1

```
5
```

## (3) Rip-up and Reroute

#### 3.1 Apply net ordering

- This is net ordering, not pin ordering. Explained later.

#### 3.2 For each net in the order found from step 3.

- Rip-up up the net:
    - Rip-up means to remove the existing route stored for the net by
       decreasing the edge utilizations corresponding to the route and updating
       the corresponding edge weights
- Next Reroute the net
    - Reroute requires single-net min-cost routing to replace the current route
       with a new route of lower cost
    - It also requires updating the edge utilization and weights for the edges
       included in the new route

```
6
```

## (3.1) Apply Net Ordering

- This step applies
- For each net _n_ , we calculate a cost as follows
    - 𝑐𝑜𝑠𝑡 𝑛 = σ∀𝑒∈𝑟𝑜𝑢𝑡𝑒(𝑛) _we_
    - where route(n) is the route stored for net n
- Sort the nets in decreasing order of their costs to create the ordering
    - First route the nets with higher cost


## Computing the Edge Weights

- Done right after generation of initial solution (before

#### RRR starts)

1. First generate initial solution for all the nets
2. Then compute the edge weights
- Done every time a net is ripped up and every time a net

#### is rerouted as an update on the affected edges


## Computing the Edge Weights

- The weight of an edge is given by 𝑤𝑒 = ℎ𝑒𝑘 × 𝑜𝑒
𝑜𝑒 : overflow of edge e: 𝑜𝑒 = max(𝑢𝑒 −𝑐𝑒, 0 )
    𝑢𝑒 : utilization of edge e: number of routes passing from e based
    on the currently-stored routes and 𝑐𝑒 is the capacity of edge e
    ℎ𝑒𝑘 : history of overflow of edge e

```
; if 𝑜𝑒 > 0
; else
```

## Computing the Edge Weights

- Before starting RRR (after generating the initial

#### solution), assume ℎ𝑒^0 = 1 for all edges to compute the

#### edge weights

- History is a function of edge utilization and can be

#### updated every time the edge utilization is updated in

#### your program


## Single-Net Min-Cost Routing

- Single-net routing is used during the RRR stage to connect

#### the pins of a net (two pins at a time based on the generated

#### pin ordering)

- Need to implement two-pin routing
- Implement any variation of single-net min-cost routing that

#### you desire

- Dijkstra's algorithm : most time consuming, may not finish execution
- Dijkstra's algorithm with “framing” option explained for Lee’s
    algorithm : helps accelerate the speed and restrict the search space
- A* (most popular one and used by modern routers)
- Pattern routing : easiest option
    - Explore among patterns such as L-shape, Z-shape, U-shape, etc.
    - Can integrate with framing idea to gradually increase the frame size as a
       function of RRR iteration to allow searching in wider area with increase in
       iteration count

```
11
```

## (4) Termination Condition

- Min time: 5 min (ensures RRR is applied)
- Max time: 30 min
- Can be anything in between based on optimizing the

#### quality metric below:

##### Q = TOF X ( 1 + RUN/15 )

- where TOF is total overflow (sum of edge overflow) of your
    final routing solution as reported by the evaluation script
- RUN is runtime of your executable in minutes on a free tux
    machine (best-tux.cae.wisc.edu)
- Notice that 15 minutes is used as a reference for your
    runtime so any runtime beyond 15 minutes will result in
    scaling up TOF accordingly

```
12
```

## Suggestion for Teamwork

1. Pin ordering (M1)
2. Generate initial solution (part 1)
3. Rip-up and reroute
    1. Edge weight calculation (M1)
    2. Net ordering (M2)
    3. Min-cost routing (M2)
4. Termination condition (M1)

```
13
```

## Format of the Executable

- **Example: ./ROUTE.exe – d=0 –n=0 <input_benchmark_name>**
    **<output_file_name>**
       - Please make sure to follow the exact above format

```
Input arguments:
```
- d=0 -n=0 : no pin ordering, no net ordering, no RRR (this is just your part 1)
- d=0 -n=1 : default pin ordering, but apply net ordering, and apply ripup and reroute
- d=1 -n=0 : apply pin ordering, but no net ordering, and no ripup and reroute (same
as part 1 but with your pin ordering enabled)
- d=1 -n=1 : apply all the features and ripup and reroute

```
14
```

## Minimum Requirements

## of the Project

- Your router should have all the components defined

#### in slide 3

- It should additionally have the expected behavior as

#### defined below based on TOF (total overflow) as

#### measured/reported by the evaluation script:

- TOF in –d=1 –n=1 case is better than –n=0 –d=
- TOF in –d=0 –n=1 case is better than –n=0 –d=0 but worse
    than –n=1 –d=
- TOF in –d=1 –n=0 case is better than –d=0 –n=0 but worse
    than –n=1 –d=

```
15
```

## Project Grading

- Project is 25% of your overall grade
- Part 1: 20% (due 3/12)
    - Full grade if passes evaluation script for the adaptec
       benchmark
- Part 2: 50% (due 4/14)
    - Based on meeting minimum requirements
- Part 3: 30% (due 4/28)
    - Based on value of Q (quality metric) of your submission
    - The quality metric Q will only be measured if the minimum
       requirements are satisfied. If they are not satisfied, looking at the
       quality metric will be irrelevant, and no grade will be given for
       this portion.

```
16
```

## What to Submit?

Part 2 deadline: 4/

1. A single zipped labeled which includes the following files:
    - (1) ece556.cpp (2) ece556.h (3) main.cpp (4) Makefile (5) a README file
    - The zipped file may include additional source files but may not include object file
       (.o) or executables
    - Include project team member names in the README file with a description of
       the worked done by each member
    - Submit only one zipped file per group
    - What else to include in the README file?
       1. Fill the table below for adaptec1, adaptec2, adaptec3 using TOF and TWL as reported by the
          evaluation script
       2. Work done by each group member

```
17
```

