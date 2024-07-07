/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "coap.h"
#include "../utilities/json_utils.c"
#include "os/dev/leds.h"
#include "contiki.h"

static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler();

typedef enum {
    OFF = 0,
    ON = 1
} State;

static State backupPumpState = OFF;

EVENT_RESOURCE(res_backup_actuator,
                  "title=\"Backup actuator\"\"PUT\";rt=\"Control\";obs",
                  res_get_handler,
                  NULL,
                  res_put_handler,
                  NULL,
                  res_event_handler);

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  
  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_header_max_age(response, res_backup_actuator.periodic->period*100000);
  coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "{\"state\": %d}", (int) backupPumpState));

  /* The coap_subscription_handler() will be called for observable resources by the REST framework. */
}
static void res_event_handler() {
  if(1) {
    coap_notify_observers(&res_backup_actuator);
  }
}
static void res_put_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    const uint8_t *payload = NULL;
    int len = 0;
    char action[30];
    char json_response[256];
    char attribute[] = "action";
    len = coap_get_payload(request, &payload);
    printf("received request\n");
    printf("%s", payload);
    
    if(len > 0) {
    
        parse_attribute_json((const char *) payload, attribute, action, sizeof(action));
        
        if(strcmp(action, "on") == 0) {
        	    
                    backupPumpState = ON;
                    
      		    leds_on(LEDS_YELLOW);
                    res_backup_actuator.trigger();
                    printf("Backup Pump switched on\n");
                    snprintf(json_response, sizeof(json_response), "{\"message\":\"Backup Pump switched on\"}");
                    
                    memcpy(buffer, json_response, strlen(json_response));
                    coap_set_payload(response, buffer, strlen(json_response));
                    coap_set_status_code(response, CHANGED_2_04);
                
            } else if(strcmp(action, "off") == 0) {
                
                backupPumpState = OFF;
      		leds_off(LEDS_YELLOW);
                res_backup_actuator.trigger();
                printf("Backup Pump switched off\n");
                
                snprintf(json_response, sizeof(json_response), "{\"message\":\"Backup Pump switched off\"}");
                memcpy(buffer, json_response, strlen(json_response));
                
                coap_set_payload(response, buffer, strlen(json_response));
                coap_set_status_code(response, CHANGED_2_04);
            } else {
                snprintf(json_response, sizeof(json_response), "{\"message\":\"Invalid Action\"}");
                memcpy(buffer, json_response, strlen(json_response));
                coap_set_payload(response, buffer, strlen(json_response));
                coap_set_status_code(response, BAD_REQUEST_4_00);
            }

    } else {
        coap_set_status_code(response, BAD_REQUEST_4_00);
    }

}
