#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openwsman/wsman-api.h>
#include <openwsman/u/libu.h>

#define WSMAN_PORT 16992

typedef struct __wsmid_identify
{
	XML_TYPE_STR ProtocolVersion;
	XML_TYPE_STR ProductVendor;
	XML_TYPE_STR ProductVersion;
} wsmid_identify;

static wsmid_identify *id;

SER_START_ITEMS(wsmid_identify)
SER_NS_STR(XML_NS_WSMAN_ID, "ProtocolVersion", 1),
SER_NS_STR(XML_NS_WSMAN_ID, "ProductVendor", 1),
SER_NS_STR(XML_NS_WSMAN_ID, "ProductVersion", 1),
SER_END_ITEMS(wsmid_identify);

static char vendor;
static char version;
static char protocol;

static WsManClient *cl;
static WsXmlDocH doc;
static client_opt_t *options;
static WsSerializerContextH cntx;

int identifySystem(){

	cl = wsmc_create(   "localhost",
                        WSMAN_PORT,
                        "/wsman",
                        "http",
                        "admin",
                        "@Linuxtar1");
    
	options = wsmc_options_init();

 	doc = wsmc_action_identify(cl, options);
    
	if (doc) {
		WsXmlNodeH soapBody = ws_xml_get_soap_body(doc);
		if (ws_xml_get_child(soapBody, 0, XML_NS_WSMAN_ID, "IdentifyResponse")) {
			cntx = ws_serializer_init();

			id = ws_deserialize(cntx,
					soapBody,
					wsmid_identify_TypeInfo, "IdentifyResponse",
					XML_NS_WSMAN_ID, NULL,
					0, 0);
			if (!id) {
					fprintf(stderr, "Serialization failed\n");
					return 1;
			}

			if (vendor){
				printf("%s\n", id->ProductVendor);
            }
			if (version){
				printf("%s\n", id->ProductVersion);
            }
			if (protocol){
				printf("%s\n", id->ProtocolVersion);
            }
			if (!protocol && !vendor && !version && id) {
				printf("\n");
				printf("%s %s supporting protocol %s\n", id->ProductVendor, 
						id->ProductVersion,id->ProtocolVersion);
			}
		}

		ws_xml_destroy_doc(doc);
	} else {
		if (wsmc_get_response_code(cl) != 200) {
			fprintf(stderr, "Connection failed. response code = %ld\n",
				wsmc_get_response_code(cl));
			if (wsmc_get_fault_string(cl)) {
				fprintf(stderr, "%s\n",
                    wsmc_get_fault_string(cl));
			}
		}
	}
    
	wsmc_options_destroy(options);
	wsmc_release(cl);  
    
    return 0;
}
