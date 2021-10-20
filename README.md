# ece252-lab2
Due: Friday October 8th, 2021.

## Goal
Fetch 50 pngs that are strips of a larger png concurrently using the cURL library from ece servers. PNG strips can come in any order and any number of occurences. All 50 strips must be retrieved and then concatenated and outputted to a file.

## Implementation
The following image shows the desired structure of our program.
![Architecture Diagram](https://git.uwaterloo.ca/kalatras/ece252-lab2/-/raw/master/bin/IMG_E90B20EF1508-1.jpeg)

### Producers
Producers will be responsible for fetching the data from the ECE servers using cURL. Producers should be created with a `server_number` parameter so they know which server to fetch the data from. Producers will be spawned by the master thread. Producers will pass structs containing a `simple_PNG` and the strip number retrieved from the header.

NOTE: The producer will use a mutex_lock operation to lock the queue! all queue operations must happen between lock/unlock statements since queue operations cannot be atomic

### Queue
The queue will be the buffer space where producers pass information the 1 or more consumer. The queue will use nodes with data type being a struct containing `simple_PNG` and the strip number of the image. Traditional queue structure will be maintained.

NOTE: all queue operations must happen between lock/unlock statements since queue operations cannot be atomic!

### Consumer(s)
The consumer thread(s) will check if the strip has been added to the master list already. If they have been, free the currently held struct from memory (since it will no longer be in the queue) and fetch the next element in the queue. The consumer section of the code must wait for elements to be added to the queue. This can be done 

NOTE: Fetching an element from the queue must be braced by lock/unlock calls! 

### Master Thread
The master thread is responsible for initialising all threads, queues, and arrays of `simple_png` types. The master thread will loop until the array has been filled, once it has it will concatenate all the pngs into a single file.

## Pseudocode for Producer/Master Thread Relationship
```
mutex_lock_t queue_mutex;

producer(url) {
    while (true) {
        fetch_png()
        mutex_lock(queue_mutex)
        enqueue(png)
        mutex_unlock(queue_mutex)
    }
}

master_thread () {
    create_queue
    png_array, png_array_size
    create n producers
    while (arr_is_not_full) {
        mutex_lock(queue_mutex)
        foo = deqeueue //if nothing is dequeued, just continue
        mutex_unlock(queue_mutex)
        consume_data(foo)
    }

    catpng()
}

```
