#include <stdio.h>
#include "log.h"

int main() {
    log_init();
    logi("LOG", "this is an info log test...\n");
    logd("LOG", "this is a debug log test...\n");
    logv("LOG", "this is a verbose log test...\n");
    logw("LOG", "this is a warning log test...\n");
    loga("LOG", "this is an assert log test...\n");
    loge("LOG", "this is an error log test...\n");
    log_deinit();
    return 0;
}
