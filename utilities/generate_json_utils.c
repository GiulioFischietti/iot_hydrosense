#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void generate_json_string(char *buffer, const char *sensor_name, const char *value) {
    sprintf(buffer, "{\"sensor_name\": \"%s\", \"value\": %s}",
                    sensor_name, value);
}

