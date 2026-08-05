#include <stdio.h>
#include <stdlib.h>
#include <prom.h>
#include <promhttp.h>
#include <microhttpd.h>

#define PORT 9091

int main(int argc, const char **argv) {
    // Initialize the default registry
    if (prom_collector_registry_default_init() != 0) {
        fprintf(stderr, "Failed to initialize default registry\n");
        return 1;
    }

    // Create a custom collector registry
    prom_collector_registry_t *custom_registry = prom_collector_registry_new("custom_registry");
    if (custom_registry == NULL) {
        fprintf(stderr, "Failed to create custom registry\n");
        return 1;
    }

    // Create a counter metric
    prom_counter_t *custom_counter = prom_counter_new("custom_counter", "A custom counter", 0, NULL);
    if (custom_counter == NULL) {
        fprintf(stderr, "Failed to create custom counter\n");
        prom_collector_registry_destroy(custom_registry);
        return 1;
    }

    // Register the counter with the custom registry
    if (prom_collector_registry_register_metric(prom_collector_registry_default(), custom_counter) != 0) {
        fprintf(stderr, "Failed to register custom counter with custom registry\n");
        prom_counter_destroy(custom_counter);
        prom_collector_registry_destroy(custom_registry);
        return 1;
    }

    // Bridge the custom registry to the default registry
    if (prom_collector_registry_bridge(custom_registry) != 0) {
        fprintf(stderr, "Failed to bridge custom registry to default registry\n");
        prom_counter_destroy(custom_counter);
        prom_collector_registry_destroy(custom_registry);
        return 1;
    }

    // Increment the custom counter
    prom_counter_inc(custom_counter, NULL);

    // Create the HTTP server to expose metrics
    struct MHD_Daemon *daemon;
    daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL);
    if (NULL == daemon) {
        fprintf(stderr, "Failed to start daemon\n");
        return 1;
    }

    printf("Serving metrics on port %d\n", PORT);
    getchar();  // Wait for user input to terminate

    // Clean up
    MHD_stop_daemon(daemon);
    prom_collector_registry_destroy(custom_registry);
    prom_counter_destroy(custom_counter);
    prom_collector_registry_destroy(PROM_COLLECTOR_REGISTRY_DEFAULT);

    return 0;
}

