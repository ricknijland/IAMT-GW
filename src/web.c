#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openwsman/wsman-api.h>
#include <openwsman/u/libu.h>

#include "include/wsman_webaccess.h"

static char            *endpoint = "http://intel.com/wbem/wscim/1/amt-schema/1/AMT_WebUIService";
static char            *method   = "RequestStateChange";
static WsManClient     *cl;
static client_opt_t    *options;
static WsXmlDocH        doc;

#define BUF_SIZE 20
#define WSMAN_PORT 16992

int webAcsCtrl(e_webmode input){
      
    //buffer with method parameters
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "RequestedState=%d", input);
    
    //init client
    cl = wsmc_create( "localhost",
        WSMAN_PORT,
        "/wsman",
        "http",
        "admin",
        "@Linuxtar1");
    
    //init transports and options
    wsmc_transport_init(cl, NULL);
    options = wsmc_options_init();
    
    //set paramater for function: 'RequestedState=input'
    wsmc_add_prop_from_str(options, buf);
    
    //invoke method with parameters and return a WsXmlDocH
    doc = wsmc_action_invoke(cl, endpoint, options, method, NULL);

    if(doc){      
        //get returnvalue to determine success 
        char* output = ws_xml_get_xpath_value(doc, "/a:Envelope/a:Body/g:RequestStateChange_OUTPUT/g:ReturnValue");
        //print response to CLI
        printf("Web Access %s %s\n",input==WEB_ENABLED?"Enable":"Disable",strncmp(output , "0", 1)?"failed":"successful");
        
        free(output);
    }else{
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
    
    //free variables/pointers
    ws_xml_destroy_doc(doc);
    wsmc_options_destroy(options);
	wsmc_release(cl); 
    
    return 0;
}
