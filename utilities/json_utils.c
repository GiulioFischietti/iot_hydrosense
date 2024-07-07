#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "coap.h"

static int parse_attribute_json(const char *json, const char *attribute, char *result, int result_size) {
    jsmn_parser parser;
    jsmntok_t tokens[10];

  
    jsmn_init(&parser);
    int json_len = strlen(json);
    int ret = jsmn_parse(&parser, json, json_len, tokens, 10);

    if (ret < 0) {
        return -1;
    }

    if (ret < 1 || tokens[0].type != JSMN_OBJECT) {
        return -1;
    }

    for (int i = 1; i < ret; i++) {
        if (tokens[i].type == JSMN_STRING) {
            if (strncmp(json + tokens[i].start, attribute, tokens[i].end - tokens[i].start) == 0) {
                //printf("string check: %s\n", json + tokens[i].start);
                int length = tokens[i + 1].end - tokens[i + 1].start;
                if (length < result_size) {
                    strncpy(result, json + tokens[i + 1].start, length);
                    result[length] = '\0';
                    return 0;
                } else {
                    return -1;
                }
            }
        }
    }

    return -1;
}
