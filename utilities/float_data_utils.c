#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static void float_to_string(char *buffer, size_t buffer_size, float valore) {
    int intero = (int)valore; 
    float frazionario = valore - intero;
    int frazionario_int = (int)(frazionario * 100); 
    
    snprintf(buffer, buffer_size, "%d.%02d", intero, frazionario_int);
}

static float generate_random_float(float min, float max) {
    float scale = rand() / (float) RAND_MAX; 
    return min + scale * (max - min);
}
