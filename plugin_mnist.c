#include "ap_core.h"
#include "app/app.h"
#include "turbospork.h"

// IMPORTANT NOTE:
// This plugin does not follow the conventions of a "proper" animated presentation plugin
// Only works on desktop

typedef struct {
    vec4d display_rect;
    vec2d out_pos;
    vec2d out_elem_size;
    vec2d out_spacing;

    vec2 prev_tile;
} mnist_input;

typedef struct {
    vec2d grid_size;
    vec2d grid_spacing;
    vec2d grid_start;
    vec2d filter_size;
} mnist_filters;

typedef struct {
    vec4d rect;

    u32 index;
} mnist_output;

static mg_arena* perm_arena = NULL;
static ts_network* nn = NULL;
static ts_tensor* img = NULL;
static ts_tensor* conv_out = NULL;
static ts_tensor* out = NULL;

static const f32 draw_kernel[9] = {
    0.125f, 0.25f, 0.125f,
    0.25f, 1.0f, 0.25f,
    0.125f, 0.25f, 0.125f,
};

void mnist_input_update(app_app* app, void* obj, f32 delta) {
    AP_UNUSED(delta);

    mnist_input* mnist = (mnist_input*)obj;

    if (GFX_IS_KEY_JUST_DOWN(app->win, GFX_KEY_R)) {
        ts_tensor_fill(img, 0.0f);
    }

    if (GFX_IS_MOUSE_DOWN(app->win, GFX_MB_LEFT)) {
        vec2 cur_tile = {
            floorf(28 * (app->mouse_pos.x - mnist->display_rect.x) / mnist->display_rect.z),
            floorf(28 * (app->mouse_pos.y - mnist->display_rect.y) / mnist->display_rect.z),
        };

        if (cur_tile.x != mnist->prev_tile.x || cur_tile.y != mnist->prev_tile.y) {
            for (u32 y_off = 0; y_off < 3; y_off++) {
                for (u32 x_off = 0; x_off < 3; x_off++) {
                    i32 x = cur_tile.x + x_off - 1;
                    i32 y = cur_tile.y + y_off - 1;

                    if (x >= 0 && x < 28 && y >= 0 && y < 28) {
                        f32* num = &img->data[x + y * 28];
                        *num += draw_kernel[x_off + y_off * 3];
                        *num = MIN(1.0f, *num);
                    }
                }
            }

            ts_tensor_copy_ip(conv_out, img);

            for (u32 i = 0; i < 3; i++) {
                ts_layer_feedforward(nn->layers[i], conv_out, NULL);
            }

            ts_network_feedforward(nn, out, img);
        }

        mnist->prev_tile = cur_tile;
    }
}

void mnist_input_draw(app_app* app, void* obj) {
    mnist_input* mnist = (mnist_input*)obj;

    // Drawing digit
    vec2 rect_size = {
        mnist->display_rect.z / 28,
        mnist->display_rect.w / 28
    };

    for (u32 y = 0; y < 28; y++) {
        for (u32 x = 0; x < 28; x++) {
            rect r = {
                mnist->display_rect.x + rect_size.x * x,
                mnist->display_rect.y + rect_size.y * y,
                rect_size.x, rect_size.y
            };

            f32 c = img->data[x + y * 28];

            draw_rectb_push(app->rectb, r, (vec4d){ c, c, c, 1.0f });
        }
    }

    // Drawing network output
    for (u32 i = 0; i < 10; i++) {
        rect r = {
            mnist->out_pos.x + mnist->out_spacing.x * i,
            mnist->out_pos.y + mnist->out_spacing.y * i,
            mnist->out_elem_size.x, mnist->out_elem_size.y
        };
        
        f32 c = out->data[i];

        draw_rectb_push(app->rectb, r, (vec4d){ c, c, c, 1.0f });
    }
}

void mnist_filters_draw(app_app* app, void* obj) {
    mnist_filters* filters = (mnist_filters*)obj;

    ts_tensor kernels = {
        .shape = { 3, 3, 32 }
    };
    kernels.data = nn->layers[1]->conv_2d_backend.kernels->data;

    vec2 pixel_size = {
        filters->filter_size.x / 3.0f,
        filters->filter_size.y / 3.0f,
    };

    if (filters->grid_size.x * filters->grid_size.y != 32) {
        return;
    }

    for (u32 y = 0; y < filters->grid_size.y; y++) {
        for (u32 x = 0; x < filters->grid_size.x; x++) {
            f32* data = &kernels.data[(x + y * (u32)filters->grid_size.x) * 9];

            f32 abs_max = fabsf(data[0]);
            for (u32 i = 1; i < 9; i++) {
                if (fabsf(data[i]) > abs_max) {
                    abs_max = fabsf(data[i]);
                }
            }

            for (u32 f_y = 0; f_y < 3; f_y++) {
                for (u32 f_x = 0; f_x < 3; f_x++) {
                    rect r = {
                        filters->grid_start.x + x * filters->grid_spacing.x + f_x * pixel_size.x,
                        filters->grid_start.y + y * filters->grid_spacing.y + f_y * pixel_size.y,
                        pixel_size.x, pixel_size.y
                    };

                    f32 num = data[f_x + f_y * 3] / abs_max;

                    vec4d col = num > 0 ? 
                        (vec4d){ 0, 0, num, 1 } : 
                        (vec4d){ -num, 0, 0, 1 };

                    draw_rectb_push(app->rectb, r, col);
                }
            }
        }
    }
}

void mnist_output_update(app_app* app, void* obj, f32 delta) {
    AP_UNUSED(delta);

    mnist_output* output = (mnist_output*)obj;

    if (GFX_IS_KEY_JUST_DOWN(app->win, GFX_KEY_D)) {
        output->index++;
        output->index %= 32;
    }
    if (GFX_IS_KEY_JUST_DOWN(app->win, GFX_KEY_A)) {
        if (output->index == 0) {
            output->index = 31;
        } else {
            output->index--;
        }
    }
}
void mnist_output_draw(app_app* app, void* obj) {
    mnist_output* output = (mnist_output*)obj;

    vec2 pixel_size = {
        output->rect.z / 26,
        output->rect.w / 26,
    };

    f32* img = &conv_out->data[output->index * 26 * 26];

    for (u32 y = 0; y < 26; y++) {
        for (u32 x = 0; x < 26; x++) {
            rect r = {
                output->rect.x + x * pixel_size.x,
                output->rect.y + y * pixel_size.y,
                pixel_size.x, pixel_size.y
            };

            f32 c = img[x + y * 26];

            draw_rectb_push(app->rectb, r, (vec4d){ c, c, c, 1.0f });
        }
    }
}

AP_EXPORT void plugin_init(marena* arena, app_app* app) {
    mga_desc desc = {
        .desired_max_size = MGA_MiB(16),
        .desired_block_size = MGA_MiB(1),
    };
    perm_arena = mga_create(&desc);

    img = ts_tensor_create(perm_arena, (ts_tensor_shape){ 28, 28, 1 });
    conv_out = ts_tensor_create(perm_arena, (ts_tensor_shape){ 26, 26, 32 });
    out = ts_tensor_create(perm_arena, (ts_tensor_shape){ 10, 1, 1 });
    nn = ts_network_load(perm_arena, TS_STR8("mnist_network.tsn"), false);

    obj_desc mnist_input_desc = {
        .name = STR("mnist_input"),
        .obj_size = sizeof(mnist_input),

        .update_func = mnist_input_update,
        .draw_func = mnist_input_draw,

        .fields = {
            { STR("rect"         ), FIELD_VEC4D, offsetof(mnist_input, display_rect ) },
            { STR("out_pos"      ), FIELD_VEC2D, offsetof(mnist_input, out_pos      ) },
            { STR("out_elem_size"), FIELD_VEC2D, offsetof(mnist_input, out_elem_size) },
            { STR("out_spacing"  ), FIELD_VEC2D, offsetof(mnist_input, out_spacing  ) },
        }
    };

    obj_desc mnist_filters_desc = {
        .name = STR("mnist_filters"),
        .obj_size = sizeof(mnist_filters),

        .draw_func = mnist_filters_draw,

        .fields = {
            { STR("grid_size"   ), FIELD_VEC2D, offsetof(mnist_filters, grid_size   ) },
            { STR("grid_spacing"), FIELD_VEC2D, offsetof(mnist_filters, grid_spacing) },
            { STR("grid_start"  ), FIELD_VEC2D, offsetof(mnist_filters, grid_start  ) },
            { STR("filter_size" ), FIELD_VEC2D, offsetof(mnist_filters, filter_size ) },
        }
    };

    obj_desc mnist_output_desc = {
        .name = STR("mnist_output"),
        .obj_size = sizeof(mnist_output),

        .update_func = mnist_output_update,
        .draw_func = mnist_output_draw,

        .fields = {
            { STR("rect"), FIELD_VEC4D, offsetof(mnist_output, rect) }
        }
    };

    obj_reg_add_desc(arena, app, app->pres->obj_reg, &mnist_input_desc);
    obj_reg_add_desc(arena, app, app->pres->obj_reg, &mnist_filters_desc);
    obj_reg_add_desc(arena, app, app->pres->obj_reg, &mnist_output_desc);
}

