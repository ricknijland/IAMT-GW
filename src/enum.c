#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openwsman/wsman-api.h>
#include <openwsman/u/libu.h>

#include "include/wsman_enumeration.h"

#define WSMAN_PORT 16992
#define BUF_SIZE 128
#define MAX_DEVICES 64
#define AMT_CLASS_LENGTH 4

typedef enum {
    CIM,
    INTEL_IPS,
    INTEL_AMT
} e_schema;

typedef enum {
    STATUS_UNKNOWN = 0,
    STATUS_OTHER = 1,
    STATUS_OK = 2,
} e_iAMT_device_status;

typedef struct {
    char *name;
    char *type;
    e_iAMT_device_status status;
} iAMT_device_t;

typedef iAMT_device_t*  iAMT_device_pt;

static iAMT_device_pt   devices[MAX_DEVICES]; //stores devices 
static iAMT_device_pt   faulty_devices[MAX_DEVICES]; //stores faulty devices
static iAMT_device_pt   unspec_devices[MAX_DEVICES]; //stores unspecified devices
static int              device_counter = 0; //keeps count of devices
static int              f_device_counter = 0; //keeps count of f_devices
static int              u_device_counter = 0; // keeps count of u_devices
static e_debug          debug_mode;

//faulty hardware list

static char            *endpoint;
static char            *cim_class = "CIM_LogicalDevice";
static WsManClient     *cl;
static client_opt_t    *options;
static e_schema        namespace;

//values to retrieve from XML response
static char            *names[3] = {    "DeviceID",
                                        "ElementName",
                                        "OperationalStatus" };

//valid iAMT namespaces                                        
static char            *namespaces[3] = {   "http://schemas.dmtf.org/wbem/wscim/1/cim-schema/2/",
                                            "http://intel.com/wbem/wscim/1/ips-schema/1/",
                                            "http://intel.com/wbem/wscim/1/amt-schema/1/"   };
                                        
//status values lookup table
static char            *status[20] = {  "Unknown", 
                                        "Other",
                                        "OK",
                                        "Degraded",
                                        "Stressed",
                                        "Predictive Failure",
                                        "Error",
                                        "Non-recoverable Error",
                                        "Starting",
                                        "Stopping",
                                        "Stopped",
                                        "In Service",
                                        "No Contact",
                                        "Lost Communication",
                                        "Aborted",
                                        "Dormant",
                                        "Supporting Entity in Error",
                                        "Completed",
                                        "Power Mode",
                                        "Relocating"    };                                                                              

//processing XML response                                        
static int process_response(WsManClient *cl, WsXmlDocH doc, void *data){
    //check response
    if(!doc){
        //200 = OK 
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
    
    /*node  Uri                  localName
     * 0:   XML_SOAP_BODY
     * 1:   XML_NS_ENUMERATION - WSENUM_PULL_RESP
     * 2:   XML_NS_ENUMERATION - WSENUM_ITEMS
     * 3:   XML_NS_WS_MAN      - WSM_ITEM
     * 4:   NULL               - NULL
     */
    
    WsXmlNodeH nodes[5]; 
    const char* uriList[] = {XML_NS_ENUMERATION, XML_NS_ENUMERATION, XML_NS_WS_MAN, NULL};
    const char* localNList[] = {WSENUM_PULL_RESP, WSENUM_ITEMS, WSM_ITEM, NULL};
    
    nodes[0] = ws_xml_get_soap_body(doc);
    
    //iterate through XML tree 
    for(int i = 0; i < 4; i++){
        if(nodes[i] != NULL){
            nodes[i+1] = ws_xml_get_child(nodes[i], 0, uriList[i], localNList[i]);
        } else {
            printf("Error processing XML response\n");
            return -2;
        }
    }
    
    //node with the class
    WsXmlNodeH child;

    //init default values
    devices[device_counter] = u_zalloc(sizeof(iAMT_device_pt));
    devices[device_counter]->name = malloc(sizeof(char) * BUF_SIZE);
    snprintf(devices[device_counter]->name, BUF_SIZE, "%s", "unspecified"); 
    devices[device_counter]->type = malloc(sizeof(char) * BUF_SIZE);
    snprintf(devices[device_counter]->type, BUF_SIZE, "%s", "unspecified"); 
    devices[device_counter]->status = 0;
    
    TODO: //check for faulty hardware
    
    if(nodes[4]){
        int index = 0;
        //retrieve values from item - normal behaviour
        if(debug_mode == DEBUG_DISABLED){
        for(int i = 0; i < 3; i++){
            index = 0;
            //select on specific values with names array
            while((child = ws_xml_get_child(nodes[4], index++ , NULL, names[i])) != NULL){
                //check for latest node in tree
                if(ws_xml_get_child(child, 0, NULL, NULL) == NULL){
                    //retrieve values from XML node
                    if(debug_mode == DEBUG_ENABLED){
                        //TODO: retrieve all info
                        printf("%s \t %s\n", ws_xml_get_node_text(child), ws_xml_get_node_text(child));
                    }else{
                        switch(i){
                            case 0: 
                                snprintf(devices[device_counter]->name, BUF_SIZE, "%s", ws_xml_get_node_text(child)); 
                                break;
                            case 1: 
                                snprintf(devices[device_counter]->type, BUF_SIZE, "%s", ws_xml_get_node_text(child)); 
                                break;
                            case 2: 
                                devices[device_counter]->status = atoi(ws_xml_get_node_text(child)); 
                                break;
                        }
                    }
                }
            } 
        }
        //debug - print all output
        }else {
            while((child = ws_xml_get_child(nodes[4], index++ , NULL, NULL)) != NULL){
                //check for latest node in tree
                if(ws_xml_get_child(child, 0, NULL, NULL) == NULL){
                    //retrieve values from XML node
                    if(debug_mode == DEBUG_ENABLED){
                        //TODO: retrieve all info
                        printf("%-35s \t %s\n", ws_xml_get_node_local_name(child), ws_xml_get_node_text(child));
                    }
                }
            }
            printf("\n");
        }
        //BIT for device status
        if(devices[device_counter]->status == STATUS_UNKNOWN){
            unspec_devices[u_device_counter] = u_zalloc(sizeof(iAMT_device_pt));
            unspec_devices[u_device_counter]->name = malloc(sizeof(char) * MAX_DEVICES);
            snprintf(unspec_devices[u_device_counter]->name, MAX_DEVICES, "%s", devices[device_counter]->name); 
            unspec_devices[u_device_counter]->type = malloc(sizeof(char) * MAX_DEVICES);
            snprintf(unspec_devices[u_device_counter]->type, MAX_DEVICES, "%s", devices[device_counter]->type); 
            unspec_devices[u_device_counter]->status = devices[device_counter]->status;
            u_device_counter++;
        } else if(devices[device_counter]->status != STATUS_OK) {
            faulty_devices[f_device_counter] = u_zalloc(sizeof(iAMT_device_pt));
            faulty_devices[f_device_counter]->name = malloc(sizeof(char) * MAX_DEVICES);
            snprintf(faulty_devices[f_device_counter]->name, MAX_DEVICES, "%s", devices[device_counter]->name); 
            faulty_devices[f_device_counter]->type = malloc(sizeof(char) * MAX_DEVICES);
            snprintf(faulty_devices[f_device_counter]->type, MAX_DEVICES, "%s", devices[device_counter]->type); 
            faulty_devices[f_device_counter]->status = devices[device_counter]->status;
            f_device_counter++;
        }
        device_counter++;
    } 
        
    return 0;
}

int enumerate(e_debug debug){
    //set debug mode according to input
    debug_mode = debug;
    //determine namespace by checking first 4 chars
    if(!strncmp(cim_class, "CIM_", AMT_CLASS_LENGTH)){
        namespace = CIM;
    } else if(!strncmp(cim_class, "AMT_", AMT_CLASS_LENGTH)){
        namespace = INTEL_AMT;
    } else if(!strncmp(cim_class, "IPS_", AMT_CLASS_LENGTH)){
        namespace = INTEL_IPS;
    }
    //construct endpoint : namespace + class 
    endpoint = malloc(sizeof(char) * BUF_SIZE);
    snprintf(endpoint, BUF_SIZE, "%s%s", namespaces[namespace], cim_class); 

    TODO: //proper password handling
    
    cl = wsmc_create( "localhost",
        WSMAN_PORT,
        "/wsman",
        "http",
        "admin",
        "@Linuxtar1");
    
    wsmc_transport_init(cl, NULL);
    options = wsmc_options_init();
    
    wsmc_set_action_option(options, FLAG_ENUMERATION_ENUM_OBJ_AND_EPR);
        
    wsmc_action_enumerate_and_pull(cl, endpoint, options, NULL, process_response, NULL);  
    
    free(endpoint);
    
    //print out devices
    if(debug_mode == DEBUG_DISABLED){
        //format results in a table
        printf("returned items: %d\n", device_counter); 
        printf("%-10s%-35s%-35s%-10s\n", "Number", "Name", "Type", "Status");
        
        for(int i = 0; i < device_counter; i++){
            printf("%-10d%-35s%-35s%-10s\n", i+1, devices[i]->name, devices[i]->type, status[devices[i]->status]);
            //free pointers of structs
            free(devices[i]->name);
            free(devices[i]->type);
            free(devices[i]);
        }
        //status == unspecified
        printf("\nunspecified\n");
        for(int j = 0; j < u_device_counter; j++){
            printf("%-10d%-35s%-35s%-10s\n", j+1, unspec_devices[j]->name, unspec_devices[j]->type, status[unspec_devices[j]->status]);
            //free pointers of structs
            free(unspec_devices[j]->name);
            free(unspec_devices[j]->type);
            free(unspec_devices[j]);
        }
        //status == anything other than unspecified or OK
        printf("\nfaulty\n");
        for(int j = 0; j < f_device_counter; j++){
            printf("%-10d%-35s%-35s%-10s\n", j+1, faulty_devices[j]->name, faulty_devices[j]->type, status[faulty_devices[j]->status]);
            //free pointers of structs
            free(faulty_devices[j]->name);
            free(faulty_devices[j]->type);
            free(faulty_devices[j]);
        }
    }
    //free pointers
    wsmc_options_destroy(options);
    wsmc_release(cl);
    
    return 0;
}
