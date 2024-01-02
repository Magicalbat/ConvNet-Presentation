
#include "ap_core.h"
#include "app/app.h"
#include "turbospork.h"

AP_EXPORT void plugin_init(marena* arena, app_app* app) {
    mga_desc desc = {
        .desired_max_size = MGA_MiB(128),
        .desired_block_size = MGA_MiB(16)
    };
    mg_arena* perm_arena = mga_create(&desc);

    ts_network* nn = ts_network_load(perm_arena, TS_STR8("mnist_network.tsn"), false);

    ts_network_summary(nn);

    ts_tensor* input = ts_tensor_create(perm_arena, (ts_tensor_shape){ 28, 28, 1 });
    ts_tensor* output = ts_tensor_create(perm_arena, (ts_tensor_shape){ 10, 1, 1 });

    ts_network_feedforward(nn, output, input);

    for (u32 i = 0; i < 10; i++) {
        printf("%f ", output->data[i]);
    }
    printf("\n");

    mga_destroy(perm_arena);
}

