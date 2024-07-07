/*
 * Copyright (c) 2014, Daniele Alessandrelli.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Erbium (Er) CoAP observe client example.
 * \author
 *      Daniele Alessandrelli <daniele.alessandrelli@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "config.h"
#include "contiki-net.h"
#include "../utilities/float_data_utils.c"
#include "../utilities/json_utils.c"
#include "coap-engine.h"
#include "coap-blocking-api.h"

#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip.h"
/* Log configuration */
#include "sys/log.h"
#include "forecast-model.h"
#include "os/dev/button-hal.h"
#include "os/dev/leds.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

/*----------------------------------------------------------------------------*/

#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)
/* Toggle interval in seconds */
#define TOGGLE_INTERVAL 30
/* The path of the resource to observe */
#define OBS_TEMP_RESOURCE_URI "temp/push"
#define OBS_VIBRATION_RESOURCE_URI "vibration/push"
#define OBS_PRESSURE_RESOURCE_URI "pressure/push"
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 25


#define UIP_IP6ADDR_STR_LEN 40
#define MAX_RETRY 3
#define FEATURE_SIZE 40
/*----------------------------------------------------------------------------*/
//static uip_ipaddr_t server_ipaddr[1]; /* holds the server ip address */
typedef enum {
    NORMAL = 0,
    BROKEN = 1
} State;
static State mainPumpState = NORMAL;

static coap_observee_t *obs_temp;
static coap_observee_t *obs_vibration;
static coap_observee_t *obs_pressure;

static char temp_payload[60] = "{ \"sensor_name\": \"temp\", \"value\": 0.0}";
static char pressure_payload[60] = "{ \"sensor_name\": \"pressure\", \"value\": 1.1}";
static char vibration_payload[60] = "{ \"sensor_name\": \"vibration\", \"value\": 2.2}";
static char flow_payload[60] = "{ \"sensor_name\": \"flow\", \"value\": 3.3}";

static coap_endpoint_t server_ep;
static coap_endpoint_t border_router_ep;


/*----------------------------------------------------------------------------*/
PROCESS(ml_model_observe_client, "Erbium Coap Observe Client ML Model");
AUTOSTART_PROCESSES(&ml_model_observe_client);
//static char endpoint[80];

/*----------------------------------------------------------------------------*/
/*
 * Handle the response to the observe request and the following notifications
 */
static float features[FEATURE_SIZE] = {0};
static float current_temp = 0;
static float current_vibration = 0;
static float current_pressure = 0;
static float current_flow = 0;

void shift_and_add(float array[], float new_value) {
    for (int i = 1; i < FEATURE_SIZE; i++) {
        array[i - 1] = array[i];
    }
    array[FEATURE_SIZE - 1] = new_value;
}

/* static float brokenfeatures[] = { 12.63744 ,  0.      , 10.09838 , 41.75347 , 12.59404 ,  0.      ,
       10.08391 , 41.75347 , 12.5651  ,  0.      , 10.12008 , 41.75347 ,
       12.72425 ,  0.      , 10.09838 , 41.84027 , 12.61574 ,  0.      ,
       10.13455 , 41.75347 , 12.77488 ,  0.      , 10.12008 , 41.84027 ,
       12.69531 ,  0.      , 10.02604 , 41.75347 , 12.88339 ,  0.      ,
       10.12731 , 41.710068, 12.51447 ,  0.      , 10.09838 , 41.66666 ,
       12.51447 ,  0.      , 10.09838 , 41.666664}; //this causes the BROKEN (1) prediction! */
static char sensor02_value_flow[5] = "0.0";
static struct etimer flow_timer;



static void
temp_notification_callback(coap_observee_t *obs, void *notification,
                      coap_notification_flag_t flag)
{
  int len = 0;
  const uint8_t *payload = NULL;

  if(notification) {
    len = coap_get_payload(notification, &payload);
  }
  switch(flag) {
  case NOTIFICATION_OK:
    strcpy(temp_payload,(const char *) payload);
    char str[6];
    parse_attribute_json(temp_payload, "value", str, sizeof(str));
    current_temp = atof(str);
    if(atof(str)==0) {
    	printf("ERROR!!!");
    }
    break;
  case OBSERVE_OK: /* server accepeted observation request */
    printf("OBSERVE_OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_NOT_SUPPORTED:
    printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case ERROR_RESPONSE_CODE:
    printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case NO_REPLY_FROM_SERVER:
    printf("NO_REPLY_FROM_SERVER: "
           "removing observe registration with token %x%x\n",
           obs->token[0], obs->token[1]);
    obs = NULL;
    break;
  }
}

static void
pressure_notification_callback(coap_observee_t *obs, void *notification,
                      coap_notification_flag_t flag)
{
  int len = 0;
  const uint8_t *payload = NULL;

  if(notification) {
    len = coap_get_payload(notification, &payload);
  }
  switch(flag) {
  case NOTIFICATION_OK:
    strcpy(pressure_payload, (const char *)  payload);
    char str[6];
    parse_attribute_json(pressure_payload, "value", str, sizeof(str));
    current_pressure = atof(str);
    if(atof(str)==0) {
    	printf("ERROR!!!");
    }
    break;
  case OBSERVE_OK: /* server accepeted observation request */
    printf("OBSERVE_OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_NOT_SUPPORTED:
    printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case ERROR_RESPONSE_CODE:
    printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case NO_REPLY_FROM_SERVER:
    printf("NO_REPLY_FROM_SERVER: "
           "removing observe registration with token %x%x\n",
           obs->token[0], obs->token[1]);
    obs = NULL;
    break;
  }
}

static void
vibration_notification_callback(coap_observee_t *obs, void *notification,
                      coap_notification_flag_t flag)
{
  int len = 0;
  const uint8_t *payload = NULL;

  if(notification) {
    len = coap_get_payload(notification, &payload);
  }
  switch(flag) {
  case NOTIFICATION_OK:
    strcpy(vibration_payload, (const char *) payload);
    char str[6];
    parse_attribute_json(vibration_payload, "value", str, sizeof(str));
    current_vibration = atof(str);
    if(atof(str)==0) {
    	printf("ERROR!!!");
    }
    break;
  case OBSERVE_OK: /* server accepeted observation request */
    printf("OBSERVE_OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_NOT_SUPPORTED:
    printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case ERROR_RESPONSE_CODE:
    printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case NO_REPLY_FROM_SERVER:
    printf("NO_REPLY_FROM_SERVER: "
           "removing observe registration with token %x%x\n",
           obs->token[0], obs->token[1]);
    obs = NULL;
    break;
  }
}




coap_endpoint_t temp_ep;
static coap_message_t temp_ip_request[1];

void temp_ip_response_handler(coap_message_t *response) {
    const uint8_t * temp_ip_payload;
    char temp_ip[40];
    char temp_ep_string[100] = "coap://[";
    
    coap_get_payload(response, &temp_ip_payload);
    parse_attribute_json((const char *) temp_ip_payload, "ip", temp_ip, sizeof(temp_ip));
    strcat(temp_ep_string, temp_ip);
    strcat(temp_ep_string, "]:5683");
    coap_endpoint_parse(temp_ep_string, strlen(temp_ep_string), &temp_ep);
    
    
    
}



static coap_endpoint_t pressure_ep;
static coap_message_t pressure_ip_request[1];

static void pressure_ip_response_handler(coap_message_t *response) {
    const uint8_t * pressure_ip_payload;
    char pressure_ip[40];
    char pressure_ep_string[100] = "coap://[";
    coap_get_payload(response, &pressure_ip_payload);
    parse_attribute_json((const char *) pressure_ip_payload, "ip", pressure_ip, sizeof(pressure_ip));
    strcat(pressure_ep_string, pressure_ip);
    strcat(pressure_ep_string, "]:5683");
    coap_endpoint_parse(pressure_ep_string, strlen(pressure_ep_string), &pressure_ep);
}


static coap_endpoint_t vibration_ep;
static coap_message_t vibration_ip_request[1];

static void vibration_ip_response_handler(coap_message_t *response) {
    char vibration_ep_string[100] = "coap://[";
    const uint8_t * vibration_ip_payload;
    char vibration_ip[40];

    coap_get_payload(response, &vibration_ip_payload);
    parse_attribute_json((const char *) vibration_ip_payload, "ip", vibration_ip, sizeof(vibration_ip));
    strcat(vibration_ep_string, vibration_ip);
    strcat(vibration_ep_string, "]:5683");
    coap_endpoint_parse(vibration_ep_string, strlen(vibration_ep_string), &vibration_ep);
}

static coap_endpoint_t backup_actuator_ep;
static coap_message_t backup_actuator_ip_request[1];

static void backup_actuator_ip_response_handler(coap_message_t *response) {
    char backup_actuator_ep_string[100] = "coap://[";
    const uint8_t * backup_actuator_ip_payload;
    char backup_actuator_ip[40];

    coap_get_payload(response, &backup_actuator_ip_payload);
    parse_attribute_json((const char *) backup_actuator_ip_payload, "ip", backup_actuator_ip, sizeof(backup_actuator_ip));
    strcat(backup_actuator_ep_string, backup_actuator_ip);
    strcat(backup_actuator_ep_string, "]:5683");
    coap_endpoint_parse(backup_actuator_ep_string, strlen(backup_actuator_ep_string), &backup_actuator_ep);
}
void
toggle_observation_temp(void)
{
  printf("Starting observation TEMP at ip:\n");
  coap_endpoint_print(&temp_ep);
  printf("\n");
  obs_temp = coap_obs_request_registration(&temp_ep, OBS_TEMP_RESOURCE_URI, temp_notification_callback,  NULL);
}

void
toggle_observation_pressure(void)
{
    printf("Starting observation PRESSURE at ip:\n");
    coap_endpoint_print(&pressure_ep);
    printf("\n");
    obs_pressure = coap_obs_request_registration(&pressure_ep, OBS_PRESSURE_RESOURCE_URI, pressure_notification_callback,  NULL);
}

void
toggle_observation_vibration(void)
{
    printf("Starting observation VIBRATION at ip:\n");
    coap_endpoint_print(&vibration_ep);
    printf("\n");
    obs_vibration = coap_obs_request_registration(&vibration_ep, OBS_VIBRATION_RESOURCE_URI, vibration_notification_callback,  NULL);
  
}


static void action_response_handler(coap_message_t *response) {
    const uint8_t *payload;
    coap_get_payload(response, &payload);
}


static struct etimer et;

static int current_attempt = 0;

void client_chunk_handler(coap_message_t *response)
{
  if (response == NULL)
  {
    LOG_ERR("Request timed out\n");
  }
  else
  {
    LOG_INFO("Registration successful\n");
    current_attempt = 3;
    return;
  }
}

static void uip_ipaddr_to_str(const uip_ipaddr_t *addr, char *buffer, size_t buffer_len)
{
  snprintf(buffer, buffer_len, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
           addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3],
           addr->u8[4], addr->u8[5], addr->u8[6], addr->u8[7],
           addr->u8[8], addr->u8[9], addr->u8[10], addr->u8[11],
           addr->u8[12], addr->u8[13], addr->u8[14], addr->u8[15]);
}

static void res_pump_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_pump_event_handler(void);
static void res_pump_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

EVENT_RESOURCE(res_main_pump,
                  "title=\"Main Pump state observer\";obs",
                  res_pump_get_handler,
                  NULL,
                  res_pump_put_handler,
                  NULL,
                  res_pump_event_handler);
                  
static void
res_pump_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_header_max_age(response, res_main_pump.periodic->period*100000);
  coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "{\"state\": %d}", (int) mainPumpState));

  /* A post_handler that handles subscriptions/observing will be called for periodic resources by the framework. */
}
static void res_pump_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *payload = NULL;
  char payload_string[64];
  coap_get_payload(request, &payload);
  strcpy(payload_string, (const char *) payload);
  
  char str[4];
  int newState;
  
  parse_attribute_json(payload_string, "status", str, sizeof(str));
  newState = atoi(str);
  
  printf("new state: %d", newState);
  
  if(newState==0) {
    leds_off(LEDS_RED);
    leds_on(LEDS_GREEN);
    mainPumpState = NORMAL;
  } else {
    leds_on(LEDS_RED);
    leds_off(LEDS_GREEN);
    mainPumpState = BROKEN;
  }
  res_main_pump.trigger();
  
}

static void res_pump_event_handler() {
  if(1) {
    coap_notify_observers(&res_main_pump);
  }
}

static void res_flow_event_handler(void);
static void res_flow_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

EVENT_RESOURCE(res_flow,
                  "title=\"Flow sensor data observer\";obs",
                  res_flow_get_handler,
                  NULL,
                  NULL,
                  NULL,
                  res_flow_event_handler);
                  
static void
res_flow_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_header_max_age(response, res_flow.periodic->period*100000);
  
  coap_set_payload(response, (uint8_t *) flow_payload, strlen(flow_payload));

  /* A post_handler that handles subscriptions/observing will be called for periodic resources by the framework. */
}

static void res_flow_event_handler() {
  if(1) {
    coap_notify_observers(&res_flow);
  }
}

static void read_sensor_data() {
  float_to_string(sensor02_value_flow, sizeof(sensor02_value_flow), generate_random_float(30.01, 70.01));
  res_flow.trigger();
  sprintf(flow_payload, "{ \"sensor_name\": \"flow\", \"value\": %s}", sensor02_value_flow);
}

PROCESS_THREAD(ml_model_observe_client, ev, data)
{
  PROCESS_BEGIN();
  coap_activate_resource(&res_main_pump, "main_pump_state");
  coap_activate_resource(&res_flow, "flow");
  etimer_set(&et, 10*CLOCK_SECOND);
  etimer_reset(&et);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  static char ip_str[UIP_IP6ADDR_STR_LEN];
  static char registration_payload[UIP_IP6ADDR_STR_LEN + 40];

  static coap_message_t registration_request[1];
  static bool mustDoRegistration = true;
  uip_ipaddr_t ipaddr;
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  printf("getting ip address...\n");

  do
  {
    static int i;
    static uint8_t state;

    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
    {
      state = uip_ds6_if.addr_list[i].state;
      if (uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
      {
        if (uip_ds6_if.addr_list[i].ipaddr.u16[0] == UIP_HTONS(0xfe80))
        {
          uip_ipaddr_to_str(&uip_ds6_if.addr_list[i].ipaddr, ip_str, sizeof(ip_str));
          break;
        }
      }
    }
  } while (ip_str[0] == '\0');
  
  leds_single_on(LEDS_YELLOW);
  if (mustDoRegistration)
  {
    printf("Registration...\n");
    do
    {
      coap_endpoint_parse("coap://[fd00::1]:5683", strlen("coap://[fd00::1]:5683"), &border_router_ep);
      coap_init_message(registration_request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(registration_request, "/register_sensor");
      sprintf(registration_payload, "{\"sensor_name\": \"flow\", \"ip\": \" %s \"}", ip_str);

      coap_set_payload(registration_request, (uint8_t *)registration_payload, strlen(registration_payload));
      current_attempt++;
      COAP_BLOCKING_REQUEST(&border_router_ep, registration_request, client_chunk_handler);
      printf("CURRENT ATTEMPT: %d\n", current_attempt);
    } while (current_attempt != MAX_RETRY);
  }
  printf("Ready.\n");

  coap_message_t db_request[1];
  coap_message_t backup_actuator_request[1];


  etimer_set(&flow_timer, 4*CLOCK_SECOND);
  etimer_reset(&flow_timer);
  
  static bool dbDataSyncEnabled = true;



  // --- REQUEST TEMPERATURE SENSOR IP --- //
  printf("REQUESTING TEMPERATURE SENSOR IP... \n");
  coap_init_message(temp_ip_request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(temp_ip_request, "/sensor_ip_by_name");
  coap_set_header_uri_query(temp_ip_request, "sensor_name=temp");
  COAP_BLOCKING_REQUEST(&border_router_ep, temp_ip_request, temp_ip_response_handler);
  

  // --- REQUEST PRESSURE SENSOR IP --- //
  printf("REQUESTING PRESSURE SENSOR IP... \n");
  coap_init_message(pressure_ip_request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(pressure_ip_request, "/sensor_ip_by_name");
  coap_set_header_uri_query(pressure_ip_request, "sensor_name=pressure");
  COAP_BLOCKING_REQUEST(&border_router_ep, pressure_ip_request, pressure_ip_response_handler);
 
  
  // --- REQUEST VIBRATION SENSOR IP --- //
  printf("REQUESTING VIBRATION SENSOR IP... \n");
  coap_init_message(vibration_ip_request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(vibration_ip_request, "/sensor_ip_by_name");
  coap_set_header_uri_query(vibration_ip_request, "sensor_name=vibration");
  COAP_BLOCKING_REQUEST(&border_router_ep, vibration_ip_request, vibration_ip_response_handler);
  
  // --- REQUEST BACKUP ACTUATOR SENSOR IP --- //
  printf("REQUESTING BACKUP ACTUATOR SENSOR IP... \n");
  coap_init_message(backup_actuator_ip_request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(backup_actuator_ip_request, "/sensor_ip_by_name");
  coap_set_header_uri_query(backup_actuator_ip_request, "sensor_name=backup_actuator");
  COAP_BLOCKING_REQUEST(&border_router_ep, backup_actuator_ip_request, backup_actuator_ip_response_handler);
  

  toggle_observation_temp();
  toggle_observation_pressure();
  toggle_observation_vibration();
	
  
  leds_single_off(LEDS_YELLOW);
  while(1) {
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&flow_timer)); 
    
    read_sensor_data();   
    
    if(dbDataSyncEnabled) {
      static char db_payload[256];
      
      strcat(db_payload, "[");
      
      strcat(db_payload, flow_payload);
      strcat(db_payload, ",");
      
      strcat(db_payload, vibration_payload);
      
      strcat(db_payload, ",");
      strcat(db_payload, temp_payload);
      
      strcat(db_payload, ",");
      strcat(db_payload, pressure_payload);
      
      strcat(db_payload, "]");
      coap_endpoint_parse("coap://[fd00::1]:5683", strlen("coap://[fd00::1]:5683"), &server_ep);
      coap_init_message(db_request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(db_request, "/update_sensor_data");
      //sprintf(dbpayload, "{\"sensor_name\": \"temp\", \"ip\": \" %s \"}", ip_str);

      coap_set_payload(db_request, (uint8_t *)db_payload, strlen(db_payload));
      COAP_BLOCKING_REQUEST(&server_ep, db_request, action_response_handler);

      db_payload[0] = '\0';

      shift_and_add(features, current_temp);
      shift_and_add(features, current_vibration);
      shift_and_add(features, current_pressure);
      shift_and_add(features, current_flow);

      if(false){
        printf("%p\n", eml_net_activation_function_strs);
      }

     	const int predicted_class = model_predict(features, FEATURE_SIZE);
     	static char actuator_payload[30];
     	
        if(predicted_class == 0) {
        mainPumpState = NORMAL;
        
      	res_main_pump.trigger();
      	leds_off(LEDS_RED);
        leds_on(LEDS_GREEN);
        printf("MAIN PUMP OK\n");
        // IF AFTER ONE PREDICTION MAIN PUMP IS OK, WE DEACTIVATE THE BACKUP PUMP
        if(mainPumpState==BROKEN) {
        
          coap_init_message(backup_actuator_request, COAP_TYPE_CON, COAP_PUT, 0);
          coap_set_header_uri_path(backup_actuator_request, "backup_actuator");
          sprintf(actuator_payload, "{\"action\":\"%s\"}", "off");
          coap_set_payload(backup_actuator_request, (uint8_t *)actuator_payload, strlen(actuator_payload));
          COAP_BLOCKING_REQUEST(&backup_actuator_ep, backup_actuator_request, action_response_handler);
        }
        
        
      } else {
        mainPumpState = BROKEN;
        res_main_pump.trigger();
      	leds_off(LEDS_GREEN);
      	leds_on(LEDS_RED);
        
        printf("MAIN PUMP MAY BREAK\n");
        printf("SWITCHING TO BACKUP PUMP...\n");
        
        
        coap_init_message(backup_actuator_request, COAP_TYPE_CON, COAP_PUT, 0);
        coap_set_header_uri_path(backup_actuator_request, "backup_actuator");
        sprintf(actuator_payload, "{\"action\":\"%s\"}", "on");
        coap_set_payload(backup_actuator_request, (uint8_t *)actuator_payload, strlen(actuator_payload));
        COAP_BLOCKING_REQUEST(&backup_actuator_ep, backup_actuator_request, action_response_handler);
        printf("DONE.\n");
        printf("\n");
        printf("PRESS THE BUTTON WHEN FIX IS COMPLETE TO SWITCH BACK TO MAIN PUMP\n");
        PROCESS_WAIT_EVENT_UNTIL((ev == button_hal_press_event) || (mainPumpState == 0));
        printf("MAIN PUMP BACK ON\n");
        etimer_reset(&flow_timer);
      	
      }
      
    }
    etimer_reset(&flow_timer);
  }

  PROCESS_END();
}
