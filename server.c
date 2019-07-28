#include <signal.h>
#include <stdio.h>

#include "open62541/open62541.h"

#include "zmq/zhelpers.h"


static void
updateCurrentTime(UA_Server *server) {
    UA_Float sin_mill = 0.f;
    UA_Variant value;
    UA_Variant_setScalar(&value, &sin_mill, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "numeric-value");
    UA_Server_writeValue(server, currentNodeId, value);
}

static void
addCurrentSinVariable(UA_Server *server, float* sin_var_addr) {
    UA_Float sin_mill = 0.f;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Numeric value");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&attr.value, &sin_mill, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "numeric-value");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "numeric-value");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, currentNodeId, parentNodeId,
                              parentReferenceNodeId, currentName,
                              variableTypeNodeId, attr, (void*)sin_var_addr, NULL);

    updateCurrentTime(server);
}

static void
beforeReadTime(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext,
               const UA_NumericRange *range, const UA_DataValue *data) {
    float sin_mill = *(float*)nodeContext;
    printf("open62541_SERVER_CALLBACK - about to send the info to the client: %f\n", sin_mill);
    UA_Float sin_uafloat = sin_mill;
    UA_Variant value;
    UA_Variant_setScalar(&value, &sin_mill, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "numeric-value");
    UA_Server_writeValue(server, currentNodeId, value);
}

static void
afterWriteTime(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               const UA_NumericRange *range, const UA_DataValue *data) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "The variable was updated");
}

static void
addValueCallbackToCurrentSinVariable(UA_Server *server) {
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "numeric-value");
    UA_ValueCallback callback;
    callback.onRead = beforeReadTime;
    callback.onWrite = afterWriteTime;
    UA_Server_setVariableNode_valueCallback(server, currentNodeId, callback);
}

// static void
// addCurrentTimeDataSourceVariable(UA_Server *server) {
//     UA_VariableAttributes attr = UA_VariableAttributes_default;
//     attr.displayName = UA_LOCALIZEDTEXT("en-US", "Current time - data source");
//     attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

//     UA_NodeId currentNodeId = UA_NODEID_STRING(1, "numeric-value-datasource");
//     UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "numeric-value-datasource");
//     UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
//     UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
//     UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

//     UA_DataSource timeDataSource;
//     UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
//                                         parentReferenceNodeId, currentName,
//                                         variableTypeNodeId, attr,
//                                         timeDataSource, NULL, NULL);
// }

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main (int argc, char *argv []) {
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    printf ("Collecting updates from the ZMQ_EMITTER server…\n");
    void *context = zmq_ctx_new();
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    int rc = zmq_connect(subscriber, "tcp://localhost:5556");
    assert (rc == 0);
    rc = zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, NULL, 0);
    assert (rc == 0);

    float sin_mill;
    addCurrentSinVariable(server, &sin_mill);
    addValueCallbackToCurrentSinVariable(server);

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server_run_startup(server);

    while(running)
    {
        int update_nbr;
        long total_temp = 0;
        char *string = s_recv(subscriber);

        sscanf(string, "%f", &sin_mill);
        printf("open62541_SERVER - value received by ZMQ_EMITTER: %f\n", sin_mill);

        UA_StatusCode retval = UA_Server_run_iterate(server, true);
        free(string);
    }

    zmq_close (subscriber);
    zmq_ctx_destroy (context);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return 0;
}