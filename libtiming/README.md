# timing 
Linux based functions to measure duration of operations with nanosecs prevision.

```

#include    "timing.h"

...


    void *t = timing_new_timer(1); // 1 for nanosecs precision


    timing_start(t);


	// operation to measure inside here

    double delta = timing_end(t);


```
