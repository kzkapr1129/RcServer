#pragma once

#define PRINT(str) fprintf(stdout, "INF: %s (%d) " str "\n", __FILE__, __LINE__)
#define LOGI(fmt,...) fprintf(stdout, "INF: %s (%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOGD(fmt,...) fprintf(stdout, "DBG: %s (%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(fmt,...) fprintf(stderr, "WAR: %s (%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(fmt,...) fprintf(stderr, "ERR: %s (%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)

