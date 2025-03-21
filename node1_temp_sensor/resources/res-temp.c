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
#include "../utilities/float_data_utils.c"
#include "../utilities/generate_json_utils.c"
#include "coap-engine.h"
#include "coap.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

PERIODIC_RESOURCE(res_temp,
                  "title=\"Temp sensor data\";obs",
                  res_get_handler,
                  NULL,
                  NULL,
                  NULL,
                  3*1000,
                  res_periodic_handler);

/*
 * Use local resource state that is accessed by res_get_handler() and altered by res_periodic_handler() or PUT or POST.
 */

static char responseString[1024];

static char sensor08_value_temp[10] = "0.0";

static void generate_sensor_data() {
  float_to_string(sensor08_value_temp, sizeof(sensor08_value_temp), generate_random_float(0.03, 24.35));
}

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /*
   * For minimal complexity, request query and options should be ignored for GET on observable resources.
   * Otherwise the requests must be stored with the observer list and passed by coap_notify_subscribers().
   * This would be a TODO in the corresponding files in contiki/apps/erbium/!
   */
  coap_set_header_content_format(response, APPLICATION_JSON);
  coap_set_header_max_age(response, res_temp.periodic->period*100000);
  generate_json_string(responseString, "temp", sensor08_value_temp);
  coap_set_payload(response, (uint8_t *)responseString, strlen(responseString));

  /* The coap_subscription_handler() will be called for observable resources by the REST framework. */
}
/*
 * Additionally, a handler function named [resource name]_handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
  generate_sensor_data();
  
  
  /* Usually a condition is defined under with subscribers are notified, e.g., large enough delta in sensor reading. */
  if(1) {
    /* Notify the registered observers which will trigger the res_get_handler to create the response. */
    coap_notify_observers(&res_temp);
  }
}
