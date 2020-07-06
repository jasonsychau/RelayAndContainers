## RelayAndContainers

This is a small template for a project run on Docker Containers.

Queuing containers is flexible by different orders of the programs in src directory.

The project is given name by the order that is given to your container orchestration.


### Programs Description

#### first

The first program is called to declare when your other services are supposed to wait for the container running first to finish the earlier portions of the container script.

If first is reused, the effect is same as above statement. However, the ports are of importance to not confuse the sought containers.

#### next-wait

The next-wait program is for only holding a queue. The container that is running next-wait is held behind and until the other program is ready to permiss these next-wait containers to go onwards with their computations. The other programs are named first (the above program) and next-pass. These two programs (first and next-pass) are given the purpose of telling other containers to go when these programs are ready to let them. The waiting programs are (this) next-wait and the next-pass.

#### next-pass

The next-pass program is to allow a queued container to go. It is called to take over the position of allowing the passing of other containers that are waiting for this next-pass. The position is taken from containers running first or next-pass with the `REQUEST_PORT` that's given to this call of next-pass.


### Example

#### Most basic example

One container is waiting for another.

| Container 1 (l:3528) | Container 2 (s:3528) |
|:---:|:---:|
| (running) | next-wait |
| (running) | (waiting for Container 1) |
| first | (receiving permission from Container 1) |
| (queued and granting permissions) | (running) |
