#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "include/wsman_webaccess.h"
#include "include/wsman_messagelog.h"
#include "include/wsman_identify.h"
#include "include/wsman_enumeration.h"

e_debug debug = DEBUG_DISABLED;

static void helper(){
    printf("Usage: app <mandatory parameters> [optional parameters]\n");
    printf("\t-e: enumerate\n");
    printf("\t-h: help\n");
    printf("\t-i: identify\n");
    printf("\t-l: retrieve messagelog\n");
    printf("\t-w: disable web-access\n");
    printf("\t-W: enable web-access\n");

}

int main(int argc, char **argv){
	int c;

	while((c = getopt (argc, argv, "dehilwW")) != -1){
		switch(c){
            case 'i':
                identifySystem();
                break;
                
            case 'd':
                debug = DEBUG_ENABLED;
                //break;
                
            case 'e':
                enumerate(debug);
                break;
            case 'w':
                webAcsCtrl(WEB_DISABLED);
                break;
            case 'W':
                webAcsCtrl(WEB_ENABLED);
                break;
            case 'l':
                getMsgLog();
                break;
			case 'h':
                helper();
                return 0;
                
            
            
			default:
				helper();
                return 1;
		}

		return 0;
    }
}
