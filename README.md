ECE556 Project Part 2

Team Members

- Prateek Tandon: Implemented pin ordering, initial solution, edge weight calculation and termination condition.
- Cole Movsessian: Implemented net ordering and min cost routing.

Routing Results

| Benchmark | -d=0 -n=0 |  | -d=0 -n=1 |  | -d=1 -n=0 |  | -d=1 -n=1 |  |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
|  | TWL | TOF | TWL | TOF | TWL | TOF | TWL | TOF |
| adaptec1 | 5,447,289 | 1,482,463 | 4,838,625 | 263,160 | 3,914,552 | 562,317 | 4,604,039 | 222,870 |
| adaptec2 | 5,151,154 | 288,301 | 3,795,618 | 779 | 3,691,565 | 88,949 | 3,461,251 | 871 |
| adaptec3 | 14,821,289 | 2,532,675 | 12,349,907 | 292,769 | 10,525,832 | 838,834 | 10,873,682 | 195,411 |
