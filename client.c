/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE) && !defined(_WRS_KERNEL)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

/**
 * Client disconnect handling
 * --------------------------
 * This example shows you how to handle a client disconnect, e.g., if the server
 * is shut down while the client is connected. You just need to call connect
 * again and the client will automatically reconnect.
 *
 * This example is very similar to the tutorial_client_firststeps.c. */

#include "open62541/open62541.h"
#include <signal.h>
#include <stdio.h>

#ifdef _WIN32
# include <windows.h>
# define UA_sleep_ms(X) Sleep(X)
#else
# include <unistd.h>
# define UA_sleep_ms(X) usleep(X * 1000)
#endif

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = 0;
}

static void
handler_sin_mill(UA_Client *client, UA_UInt32 subId, void *subContext,
                           UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "open62541_CLIENT - received data from open62541_SERVER: ");
    printf("%f\n", *(float*)value->value.data);
}

// static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Inactivity for subscription %u", subId);
}

static void
stateCallback (UA_Client *client, UA_ClientState clientState) {
    switch(clientState) {
        case UA_CLIENTSTATE_DISCONNECTED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "The client is disconnected");
        break;
        case UA_CLIENTSTATE_CONNECTED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A TCP connection to the server is open");
        break;
        case UA_CLIENTSTATE_SECURECHANNEL:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A SecureChannel to the server is open");
        break;
        case UA_CLIENTSTATE_SESSION:{
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A session with the server is open");
            /* A new session was created. We need to create the subscription. */
            /* Create a subscription */
            UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
            UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                                    NULL, NULL, deleteSubscriptionCallback);

            if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Create subscription succeeded, id %u", response.subscriptionId);
            else
                return;

            /* Add a MonitoredItem */
            UA_MonitoredItemCreateRequest monRequest =
                UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(1, "numeric-value"));

            UA_MonitoredItemCreateResult monResponse =
                UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                          UA_TIMESTAMPSTORETURN_BOTH,
                                                          monRequest, NULL, handler_sin_mill, NULL);
            if(monResponse.statusCode == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Monitoring numeric-value', id %u", monResponse.monitoredItemId);
        }
        break;
        case UA_CLIENTSTATE_SESSION_RENEWED:
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "A session with the server is open (renewed)");
            /* The session was renewed. We don't need to recreate the subscription. */
        break;
    }
    return;
}

int main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ClientConfig config = UA_ClientConfig_default;
    /* Set stateCallback */
    config.stateCallback = stateCallback;
    config.subscriptionInactivityCallback = subscriptionInactivityCallback;

    UA_Client *client = UA_Client_new(config);

    /* Endless loop runAsync */
    while (running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_USERLAND, "Not connected. Retrying to connect in 1 second");
            /* The connect may timeout after 5 seconds (default timeout) or it may fail immediately on network errors */
            /* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
            UA_sleep_ms(1000);
            continue;
        }

        UA_Client_runAsync(client, 1000);
    };

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
