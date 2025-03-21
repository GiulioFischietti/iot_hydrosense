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
 *      Erbium (Er) CoAP Engine example.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"

#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip.h"
#include "coap-blocking-api.h"
#include "os/dev/leds.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP
/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_backup_actuator;

#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 256
#undef COAP_MAX_OPEN_TRANSACTIONS
#define COAP_MAX_OPEN_TRANSACTIONS 4
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 10
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 10
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 240

#define UIP_IP6ADDR_STR_LEN 40
#define MAX_RETRY 3

PROCESS(backup_coap_server, "Temp Erbium Server");
AUTOSTART_PROCESSES(&backup_coap_server);
static struct etimer et;

static int current_attempt = 0;
static coap_endpoint_t border_router_ep;
static void uip_ipaddr_to_str(const uip_ipaddr_t *addr, char *buffer, size_t buffer_len)
{
  snprintf(buffer, buffer_len, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
           addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3],
           addr->u8[4], addr->u8[5], addr->u8[6], addr->u8[7],
           addr->u8[8], addr->u8[9], addr->u8[10], addr->u8[11],
           addr->u8[12], addr->u8[13], addr->u8[14], addr->u8[15]);
}
static void client_chunk_handler(coap_message_t *response)
{
  if (response == NULL)
  {
    LOG_ERR("[BACKUP_ACTUATOR]  Request timed out\n");
  }
  else
  {
    LOG_INFO("[BACKUP_ACTUATOR] Registration successful\n");
    current_attempt = 3;
    return;
  }
}

PROCESS_THREAD(backup_coap_server, ev, data)
{

  PROCESS_BEGIN();
  etimer_set(&et, 10*CLOCK_SECOND);
  etimer_reset(&et);
  
  
  static char ip_str[UIP_IP6ADDR_STR_LEN];
  static char registration_payload[UIP_IP6ADDR_STR_LEN + 60];

  static coap_message_t registration_request[1];
  static bool mustDoRegistration = true;
  uip_ipaddr_t ipaddr;
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  printf("BACKUP_ACTUATOR: Getting ip address...\n");

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
    printf("[BACKUP_ACTUATOR] Registration...\n");
    do
    {   
      coap_endpoint_parse("coap://[fd00::1]:5683", strlen("coap://[fd00::1]:5683"), &border_router_ep);
      coap_init_message(registration_request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(registration_request, "/register_sensor");
      sprintf(registration_payload, "{\"sensor_name\": \"backup_actuator\", \"ip\": \" %s \"}", ip_str);

      coap_set_payload(registration_request, (uint8_t *)registration_payload, strlen(registration_payload));
      current_attempt++;
      COAP_BLOCKING_REQUEST(&border_router_ep, registration_request, client_chunk_handler);
      printf("[BACKUP_ACTUATOR] CURRENT ATTEMPT: %d\n", current_attempt);
      
    } while (current_attempt != MAX_RETRY);
  }
  printf("[BACKUP_ACTUATOR] Ready.\n");
  leds_single_off(LEDS_YELLOW);
  
  coap_activate_resource(&res_backup_actuator, "backup_actuator");

  while(1) {
    PROCESS_WAIT_EVENT();
  }                             

  PROCESS_END();
}
