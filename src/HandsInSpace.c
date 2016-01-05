#include <pebble.h>

static Window *window;
static Layer *hand_layer;
static GFont hour_font;

static struct {
    int16_t second, minute, hour;
} angles;


#ifdef PBL_COLOR
    #define BG_COL GColorWhite
    #define HOUR_COL GColorBlack
    #define HOUR_HAND_COL GColorGreen
    #define MIN_HAND_COL GColorVividCerulean
    #define SEC_HAND_COL GColorRed
    #define HOUR_HAND_BORDER GColorBlack
    #define MIN_HAND_BORDER GColorBlack
    #define SEC_HAND_BORDER GColorBlack
#else
    #define BG_COL GColorWhite
    #define HOUR_COL GColorBlack
    #define HOUR_HAND_COL GColorBlack
    #define MIN_HAND_COL GColorBlack
    #define SEC_HAND_COL GColorBlack
    #define HOUR_HAND_BORDER GColorBlack
    #define MIN_HAND_BORDER GColorBlack
    #define SEC_HAND_BORDER GColorBlack
#endif

 
static void window_load(Window *window);
static void window_unload(Window *window);
static void window_load(Window *window);
static void timer_handler(struct tm *current, TimeUnits units);
static void update_hand_angles();
static void hand_layer_update(Layer *layer, GContext *ctx);
static void draw_hour_markers(GContext *ctx, GPoint center);
static void draw_second_hand(GContext *ctx, GPoint center, int angle);
static void draw_minute_hand(GContext *ctx, GPoint center, int angle);
static void draw_hour_hand(GContext *ctx, GPoint center, int angle);
static GPoint get_point(GPoint center, int radius, int degrees);


int main()
{
    hour_font = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_HOUR_HAND_FONT_28)
    );

    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    window_stack_push(window, true);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p",
        window);

    app_event_loop();

    window_destroy(window);
}


static void window_load(Window *window)
{
    window_set_background_color(window, BG_COL);
    update_hand_angles();
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    hand_layer = layer_create(bounds);
    layer_set_update_proc(hand_layer, hand_layer_update);

    layer_add_child(window_layer, hand_layer);

    tick_timer_service_subscribe(SECOND_UNIT, timer_handler);
}

static void window_unload(Window *window)
{
    tick_timer_service_unsubscribe();
}

static void timer_handler(struct tm *current, TimeUnits units)
{
    update_hand_angles();
    layer_mark_dirty(hand_layer);
}

/* Update the angles struct with the current time. */
static void update_hand_angles()
{
    time_t cur = time(NULL);
    struct tm *tick = localtime(&cur);

    angles.hour = TRIG_MAX_ANGLE * (tick->tm_hour % 12) / 12;
    angles.minute = TRIG_MAX_ANGLE * tick->tm_min / 60;
    angles.second = TRIG_MAX_ANGLE * tick->tm_sec / 60;
}

/* Draw the watchface. */
static void hand_layer_update(Layer *layer, GContext *ctx)
{
    update_hand_angles();
    GRect bounds = layer_get_bounds(layer);

    GPoint center = GPoint(
        bounds.size.w / 2 + bounds.origin.x,
        bounds.size.h / 2 + bounds.origin.y
    );

    draw_hour_markers(ctx, center);

    time_t cur = time(NULL);
    struct tm *tick = localtime(&cur);

    draw_minute_hand(ctx, center, angles.minute);
    draw_hour_hand(ctx, center, angles.hour);
    draw_second_hand(ctx, center, angles.second);
}

/* Draw hour markers (that's the numbers around the edge of the watchface) */
static void draw_hour_markers(GContext *ctx, GPoint center)
{
    char buf[32];

    for(int hour = 1; hour <= 12; hour++)
    {

        GPoint point = get_point(center, 64, (hour % 12) * 30);
        graphics_context_set_text_color(ctx, HOUR_COL);

        snprintf(buf, 32, "%d", hour);
        graphics_draw_text(ctx, buf, hour_font,
            GRect(point.x - 16, point.y - 19, 32, 30), GTextOverflowModeFill,
            GTextAlignmentCenter, NULL);
    }
}

/* Draw the second hand */
static void draw_second_hand(GContext *ctx, GPoint center, int angle)
{
    static GPath *path = NULL;
    static GPathInfo info;

    static GPoint points[] = {
        {0, -15},
        {-2, -15},
        {-2, -55},
        {0, -60},
        {2, -55},
        {2, -15},
        {0, -15}
    };

    if(!path)
    {
        info.num_points = sizeof(points) / sizeof(GPoint);
        info.points = points;
        path = gpath_create(&info);
    }

    gpath_move_to(path, center);
    gpath_rotate_to(path, angle);

    graphics_context_set_fill_color(ctx, SEC_HAND_COL);
    graphics_context_set_stroke_color(ctx, SEC_HAND_BORDER);

#ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, 2);
#endif

    gpath_draw_outline(ctx, path);
    gpath_draw_filled(ctx, path);
}

/* Draw the minute hand. */
static void draw_minute_hand(GContext *ctx, GPoint center, int angle)
{
    static GPath *path = NULL;
    static GPathInfo info;

    static GPoint points[] = {
        {0, -15},
        {-3, -15},
        {-3, -45},
        {-10, -35},
        {-0, -60},
        {10, -35},
        {3, -45},
        {3, -15},
        {0, -15}
    };

    if(!path)
    {
        info.num_points = sizeof(points) / sizeof(GPoint);
        info.points = points;
        path = gpath_create(&info);
    }

#ifdef PBL_COLOR
    graphics_context_set_antialiased(ctx, true);
#endif

    gpath_move_to(path, center);
    gpath_rotate_to(path, angle);

    graphics_context_set_fill_color(ctx, MIN_HAND_COL);
    graphics_context_set_stroke_color(ctx, MIN_HAND_BORDER);

#ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, 3);
#endif
    gpath_draw_outline(ctx, path);
    gpath_draw_filled(ctx, path);
}

/* Draw the hour hand */
static void draw_hour_hand(GContext *ctx, GPoint center, int angle)
{
    static GPath *path = NULL;
    static GPathInfo info;

    static GPoint points[] = {
        {0, -15},
        {-3, -15},
        {-3, -27},
        {-10, -20},
        {-0, -35},
        {10, -20},
        {3, -27},
        {3, -15},
        {0, -15}
    };

    if(!path)
    {
        info.num_points = sizeof(points) / sizeof(GPoint);
        info.points = points;
        path = gpath_create(&info);
    }

    gpath_move_to(path, center);
    gpath_rotate_to(path, angle);

    graphics_context_set_fill_color(ctx, HOUR_HAND_COL);
    graphics_context_set_stroke_color(ctx, HOUR_HAND_BORDER);
#ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, 3);
#endif

    gpath_draw_outline(ctx, path);
    gpath_draw_filled(ctx, path);
}

static GPoint get_point(GPoint center, int radius, int degrees)
{
    int angle = DEG_TO_TRIGANGLE(degrees);
    return GPoint(
        sin_lookup(angle) * radius / TRIG_MAX_ANGLE + center.x,
        -cos_lookup(angle) * radius / TRIG_MAX_ANGLE + center.y
    );
}

