ECE556 Project Part 3

Team Members

- Prateek Tandon: Implemented pin ordering, initial solution, edge weight calculation and termination condition.
- Cole Movsessian: Implemented net ordering and min cost routing.

P2 Routing Results

| Benchmark | -d=0 -n=0 |  | -d=0 -n=1 |  | -d=1 -n=0 |  | -d=1 -n=1 |  |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
|  | TWL | TOF | TWL | TOF | TWL | TOF | TWL | TOF |
| adaptec1 | 5,176,643 | 1,262,989 | 4,568,649 | 172,600 | 3,803,442 | 509,827 | 4,388,507 | 153,139 |
| adaptec2 | 4,978,847 | 216,881 | 3,417,482 | 623 | 3,594,231 | 79,394 | 3,287,613 | 716 |
| adaptec3 | 14,135,345 | 2,161,636 | 11,386,364 | 95,174 | 10,291,300 | 800,193 | 10,648,542 | 86,208 |

Improvements documented in Improvement_Report.pdf
