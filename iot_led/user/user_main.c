// Includes
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

// Defines
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

// Variables
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
void udp_init();

static volatile os_timer_t some_timer; // Some timer
uint32_t ledon = 0;

void some_timerfunc(void *arg) {
	return;
	
    //Do blinky stuff
    if (ledon)
    {
        //Set GPIO2 to HIGH
        gpio_output_set(BIT2, 0, BIT2, 0);
        ledon=0;
    }
    else
    {
        //Set GPIO2 to LOW
        gpio_output_set(0, BIT2, BIT2, 0);
        ledon=1;
    }
}

LOCAL void ICACHE_FLASH_ATTR udp_recv(void *arg, char *pusrdata, unsigned short length) {
    os_printf("Received data: %s\n\r", pusrdata);
    
    if(strncmp(pusrdata, "ADC", 3) == 0){
    	// ADC Value Request
    	os_printf("Got ADC Request\n\r");
		
		// Cast arg to espconn
		struct espconn* udp = arg;
		
		// Reply with ADC data
		char buf[10];
		int len = os_sprintf(&buf, "{ADC: %d}\n", system_adc_read());
		espconn_sent(udp, buf, len);
    } else if(strncmp(pusrdata, "LED", 3) == 0){
    	// LED Toggle Request
		if (ledon) {
		    //Set GPIO2 to HIGH
		    gpio_output_set(BIT2, 0, BIT2, 0);
		    ledon=0;
		} else {
		    //Set GPIO2 to LOW
		    gpio_output_set(0, BIT2, BIT2, 0);
		    ledon=1;
		}
    } else if(strncmp(pusrdata, "RSSI", 4) == 0) {
    	// Get RSSI data
    	char buf[10];
   		int len = os_sprintf(&buf, "{RSSI: %d}\n", wifi_station_get_rssi());
		struct espconn* udp = arg;
    	espconn_sent(udp, buf, len);
    } else {
    	struct espconn* udp = arg;
    	espconn_sent(udp, "ERROR", 5);
    }
    

        

}

void user_set_station_config( void ){
	char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;
    stationConf.bssid_set = 0; //need not check MAC address of AP
   
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);
    
    wifi_station_set_auto_connect(1);
}

LOCAL struct espconn udpconn;
void udp_init() {
        memset(&udpconn, 0, sizeof(struct espconn));
        udpconn.type = ESPCONN_UDP;
        udpconn.state = ESPCONN_NONE; // Becuase UDP
        udpconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
        udpconn.proto.udp->local_port = 2500;
        udpconn.proto.udp->remote_port = 2500;
        espconn_regist_recvcb(&udpconn, udp_recv);
        espconn_accept(&udpconn);
        
        // Create the UDP connection
        espconn_create( &udpconn );
        
        os_printf("UDP Setup\n\r");
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {
	// Set the USART speed to be 9600 baud for debug prints
    uart_div_modify(0, UART_CLK_FREQ / 9600);
    
    // Set the OPMODE
	wifi_set_opmode(STATION_MODE);
	
	user_set_station_config();
	
	//Set GPIO2 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    
    //Set GPIO2 low
    gpio_output_set(BIT2, 0, BIT2, 0);
	
	//Disarm timer
    os_timer_disarm(&some_timer);
 
    //Setup timer
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
 
    //Arm the timer
    //&some_timer is the pointer
    //1000 is the fire time in ms
    //0 for once and 1 for repeating
    os_timer_arm(&some_timer, 500, 1);
    
    udp_init();
}
