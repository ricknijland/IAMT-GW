#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "include/wsman_messagelog.h"
#include "include/decode_base64.h"

#include <openwsman/wsman-api.h>
#include <openwsman/u/libu.h>

#define WSMAN_PORT 16992
#define LOG_SIZE   20
#define BUF_SIZE   64
#define BASE64_LENGTH 28

typedef struct {
    char *time;
    int data[BUF_SIZE];
} iAMT_logrecord_t;

typedef iAMT_logrecord_t*  iAMT_logrecord_pt;

static char            *endpoint   =    "http://intel.com/wbem/wscim/1/amt-schema/1/AMT_MessageLog";
static char            *method     =    "GetRecords";
static char            *selectors  =    "Name=Intel(r) AMT:MessageLog 1&CreationClassName=AMT_MessageLog";
static char            *properties;
static WsManClient     *cl;
static client_opt_t    *options;
static WsXmlDocH        doc;

static int              maxreadrecords  = 10;
static int              iterationidentifier = 1;

static iAMT_logrecord_pt records[BUF_SIZE];

static char            *entity[100]     = {""};
static char            *severity[35]    = {0};

void set_lookuptables();

int getMsgLog(){
    //lookup tables for codes in response
    set_lookuptables();
    
    properties = malloc(sizeof(char)* BUF_SIZE);
    
    //init client
    cl = wsmc_create( "localhost",
        WSMAN_PORT,
        "/wsman",
        "http",
        "admin",
        "@Linuxtar1");
    
    wsmc_transport_init(cl, NULL);
    options = wsmc_options_init();
    //add selectors to determine unique instance
    wsmc_add_selectors_from_str(options,selectors);
    //add parameters for function
    
    snprintf(properties, BUF_SIZE, "%s%d&%s%d", "IterationIdentifier=", iterationidentifier, "MaxReadRecords=", maxreadrecords);
    wsmc_add_prop_from_str(options, properties);
    free(properties);
        
    //run method
    doc = wsmc_action_invoke(cl, endpoint, options, method, NULL);

    printf("%-10s%-20s%-15s%-25s\n", "Event", "Time", "Severity", "Source");
          
        //process response
        if(doc){
            
            WsXmlNodeH output = ws_xml_get_soap_body(doc);
            //get log instance
            output = ws_xml_get_child(output, 0, NULL, NULL);
            
            u_int32_t timeStamp;
            
            WsXmlNodeH node;
            
            
            for(int i = iterationidentifier; i <= maxreadrecords; i++){
                
                records[i] = malloc(sizeof(iAMT_logrecord_t));
                /* response format of a log records
                 *  response[0]                      : IterationIdentifier
                 *  response[1]                      : EndOfLog
                 *  response[2-(2+maxreadrecords)]   : record in base64 format
                 */
                //i + 1 = 2 : index of first record
                node = ws_xml_get_child(output, i + 1, NULL, NULL);
                //response in base64 format, decode first
                unsigned char* log = b64_decode(ws_xml_get_node_text(node), BASE64_LENGTH);
                
                /* response format of a log record u_int8
                * log[0-3]     : timeStamp - little endian
                * log[4]       : DeviceAddress
                * log[5]       : EventSensorType
                * log[6]       : EventType
                * log[7]       : EventOffset
                * log[8]       : EventSourceType
                * log[9]       : EventSeverity
                * log[10]      : SensorNumber
                * log[11]      : Entity
                * log[12]      : EntityInstance
                * log[13-20]   : EventData
                */ 
                                
                //put logdata in data[]
                for(int j = 0; j < BASE64_LENGTH; j++){
                    records[i]->data[j] = log[j];
                }
                free(log);
                //epoch timestamp in little endian
                u_int32_t timeStamp = 0;
                timeStamp = (records[i]->data[3] << 24 | records[i]->data[2] << 16 | records[i]->data[1] << 8 | records[i]->data[0]);
                time_t time = timeStamp;
                
                struct tm * timeinfo = localtime(&time);
                records[i]->time = malloc(sizeof(char)*BUF_SIZE);
                strftime(records[i]->time, BUF_SIZE, "%d/%m/%y %R", timeinfo);
            }
            for(int i = iterationidentifier; i <= maxreadrecords; i++){
                printf("%-10d%-20s%-15s%-25s\n", i, records[i]->time, severity[records[i]->data[9]], entity[records[i]->data[11]]);
                free(records[i]->time);
                free(records[i]); 
            }
        } else{
            if (wsmc_get_response_code(cl) != 200) {
                fprintf(stderr, "Connection failed. response code = %ld\n",
					wsmc_get_response_code(cl));
                if (wsmc_get_fault_string(cl)) {
                    fprintf(stderr, "%s\n",
                    wsmc_get_fault_string(cl));
                }
                return -1;
            }
        }
    //free variables
    ws_xml_destroy_doc(doc);
    
    wsmc_options_destroy(options);
	wsmc_release(cl);  
    
    return 0;
}

void set_lookuptables(){
    //PET specification 
    severity[0] = "unspecified";
    severity[1] = "monitor";
    severity[2] = "information";
    severity[4] = "OK";
    severity[8] = "warning";
    severity[16] = "Critical";
    severity[32] = "non-recoverable";

    entity[0] = "Unspecified";
    entity[1] = "Other";
    entity[2] = "Unknown (unspecified)";
    entity[3] = "Processor";
    entity[4] = "Disk or disk bay";
    entity[5] = "Peripheral bay";
    entity[6] = "System Management Module";
    entity[7] = "System board";
    entity[8] = "Memory module";
    entity[9] = "Processor module";
    entity[10] = "Power supply";
    entity[11] = "Add-in card";
    entity[0x23] = "OS";
    entity[0x26] = "Mgmt device";
    entity[32] = "Memory device";
    entity[34] = "BIOS";
}
