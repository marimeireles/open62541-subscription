#include <sys/time.h>
#include <math.h>

#include "zmq/zhelpers.h"

float sin_current_mill()
{
    struct timeval  tv;
    gettimeofday(&tv, NULL);

    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

    return sin(time_in_mill);
}

int main (void)
{
    //  Prepare context and publisher
    void *context = zmq_ctx_new ();
    void *publisher = zmq_socket (context, ZMQ_PUB);
    int rc = zmq_bind (publisher, "tcp://*:5556");
    assert (rc == 0);

    srandom ((unsigned) time (NULL));
    while (1) {
        float sin;
        sin = sin_current_mill();

        //  Send message to all subscribers
        char update [20];
        printf("ZMQ_EMITTER: %f\n", sin);
        sprintf (update, "%05f", sin);
        s_send (publisher, update);

        usleep(1000);
    }
    zmq_close (publisher);
    zmq_ctx_destroy (context);
    return 0;
}
