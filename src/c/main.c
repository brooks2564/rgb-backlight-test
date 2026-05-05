#include <pebble.h>

static Window *s_window;
static Layer *s_canvas_layer;

static int s_rgb[3] = {128, 128, 128};
static int s_selected = 0;

// Touch tracking
static int s_touch_last_x = -1;
static int s_touch_row = -1;     // which row is being dragged
static int s_rows_top;
static int s_row_h;

static void update_backlight(void) {
#ifdef PBL_RGB_BACKLIGHT
    uint32_t packed = ((uint32_t)s_rgb[0] << 16) |
                      ((uint32_t)s_rgb[1] << 8)  |
                       (uint32_t)s_rgb[2];
    light_set_color_rgb888(packed);
    light_enable(true);
#endif
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Color swatch
    int swatch_h = 44;
    GRect swatch = GRect(8, 6, w - 16, swatch_h);
    GColor preview = GColorFromRGB((uint8_t)s_rgb[0], (uint8_t)s_rgb[1], (uint8_t)s_rgb[2]);
    graphics_context_set_fill_color(ctx, preview);
    graphics_fill_rect(ctx, swatch, 6, GCornersAll);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_round_rect(ctx, swatch, 6);

    static const char *labels[] = {"R", "G", "B"};
    static const uint8_t label_argbs[] = {
        GColorRedARGB8, GColorGreenARGB8, GColorBlueARGB8
    };

    int rows_top = 6 + swatch_h + 8;
    int rows_h = h - rows_top - 6;
    int row_h = rows_h / 3;

    for (int i = 0; i < 3; i++) {
        int ry = rows_top + i * row_h;
        GRect row_rect = GRect(6, ry, w - 12, row_h - 3);

        if (i == s_selected) {
            graphics_context_set_fill_color(ctx, GColorDarkGray);
            graphics_fill_rect(ctx, row_rect, 4, GCornersAll);
        }

        GColor lc = (GColor){.argb = label_argbs[i]};
        graphics_context_set_text_color(ctx, lc);
        graphics_draw_text(ctx, labels[i],
            fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
            GRect(row_rect.origin.x + 8, row_rect.origin.y + 6, 24, 34),
            GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

        char val_buf[8];
        snprintf(val_buf, sizeof(val_buf), "%d", s_rgb[i]);
        graphics_context_set_text_color(ctx, GColorWhite);
        graphics_draw_text(ctx, val_buf,
            fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS),
            GRect(row_rect.origin.x + 38, row_rect.origin.y + 3, 110, 42),
            GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

        if (i == s_selected) {
            graphics_context_set_text_color(ctx, GColorYellow);
            graphics_draw_text(ctx, ">",
                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                GRect(row_rect.origin.x + row_rect.size.w - 22,
                      row_rect.origin.y + 12, 18, 24),
                GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
        }
    }
}

static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void touch_handler(const TouchEvent *event, void *context) {
    if (event->type == TouchEvent_Touchdown) {
        // Determine which row was touched
        int row = (event->y - s_rows_top) / s_row_h;
        if (row < 0 || row > 2) {
            s_touch_row = -1;
            return;
        }
        s_touch_row = row;
        s_touch_last_x = event->x;
        // Select the touched row
        s_selected = row;
        layer_mark_dirty(s_canvas_layer);

    } else if (event->type == TouchEvent_PositionUpdate) {
        if (s_touch_row < 0) return;
        int dx = event->x - s_touch_last_x;
        s_touch_last_x = event->x;
        s_rgb[s_touch_row] = clamp(s_rgb[s_touch_row] + dx, 0, 255);
        update_backlight();
        layer_mark_dirty(s_canvas_layer);

    } else if (event->type == TouchEvent_Liftoff) {
        s_touch_row = -1;
        s_touch_last_x = -1;
    }
}

static void up_handler(ClickRecognizerRef r, void *ctx) {
    if (s_rgb[s_selected] < 250) s_rgb[s_selected] += 5;
    else s_rgb[s_selected] = 255;
    update_backlight();
    layer_mark_dirty(s_canvas_layer);
}

static void down_handler(ClickRecognizerRef r, void *ctx) {
    if (s_rgb[s_selected] > 5) s_rgb[s_selected] -= 5;
    else s_rgb[s_selected] = 0;
    update_backlight();
    layer_mark_dirty(s_canvas_layer);
}

static void select_handler(ClickRecognizerRef r, void *ctx) {
    s_selected = (s_selected + 1) % 3;
    layer_mark_dirty(s_canvas_layer);
}

static void click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 80, up_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 80, down_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_handler);
}

static void window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root);

    // Precompute row geometry for touch hit-testing
    int swatch_h = 44;
    s_rows_top = 6 + swatch_h + 8;
    int rows_h = bounds.size.h - s_rows_top - 6;
    s_row_h = rows_h / 3;

    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(root, s_canvas_layer);

    touch_service_subscribe(touch_handler, NULL);
    update_backlight();
}

static void window_unload(Window *window) {
    touch_service_unsubscribe();
    layer_destroy(s_canvas_layer);
#ifdef PBL_RGB_BACKLIGHT
    light_enable(false);
#endif
}

static void init(void) {
    s_window = window_create();
    window_set_background_color(s_window, GColorBlack);
    window_set_window_handlers(s_window, (WindowHandlers){
        .load = window_load,
        .unload = window_unload,
    });
    window_set_click_config_provider(s_window, click_config_provider);
    window_stack_push(s_window, true);
}

static void deinit(void) {
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
