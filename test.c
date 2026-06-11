#include <stdio.h>
#include <stdint.h>

int main() {
    uint8_t breath_val = 0;
    int8_t breath_dir = 5;
    for (int i = 0; i < 10; i++) {
        printf("%d ", breath_val);
        breath_val += breath_dir;
        if (breath_val >= 250 || breath_val <= 5) breath_dir = -breath_dir;
    }
    printf("\n");
    return 0;
}
