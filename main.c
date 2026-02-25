#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define da_reserve(da, expected_capacity)                                              \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = 1024;                                                     \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));     \
            assert((da)->items != NULL && "Buy more RAM lol");                             \
        }                                                                                  \
    } while (0)

// Append an item to a dynamic array
#define da_append(da, item)                \
    do {                                       \
        da_reserve((da), (da)->count + 1); \
        (da)->items[(da)->count++] = (item);   \
    } while (0)

#define SCALE 400
#define WIDTH (3*SCALE)
#define HEIGHT (2*SCALE)

typedef struct {
    Vector2 *items;
    size_t count;
    size_t capacity;
} Vectors;

typedef struct {
    union {
        struct {
            float h, s, v;
        } hsv;
        struct {
            size_t r, g, b;
        } rgb;
    };
    size_t count;
} Sum;

typedef enum {
    CS_RGB = 0,
    CS_HSV,
    CS_LEN,
} ColourSpace;

static const char *cs_names[] = {
    "RGB",
    "HSV"
};

#define MIN_DIST 15

bool conflicts(Vectors *points, Vector2 point) {
    for (size_t i = 0; i < points->count; ++i) {
        int dx = points->items[i].x - point.x;
        int dy = points->items[i].y - point.y;
        int dist_sq = dx*dx + dy*dy;
        if (dist_sq < MIN_DIST*MIN_DIST) return true;
    }
    return false;
}

size_t randomise(Vectors *points, size_t count) {
    size_t added = 0;
    size_t tries = count *2;
    for (; added < count && tries > 0; tries -= 1) {
        Vector2 pos = {
            GetRandomValue(0, WIDTH),
            GetRandomValue(0, HEIGHT),
        };
        if (conflicts(points, pos)) continue;
        da_append(points, pos);
        added += 1;
    }
    return added;
}

void fill_random(Vectors *points) {
    size_t added;
    do {
        added = randomise(points, 100);
    } while (added > 0);
}

void voronoi(Texture2D texture, Vectors points, Image img, ColourSpace colour_space) {
    if (points.count == 0) return;

    Sum sums[points.count];
    memset(sums, 0, sizeof(Sum)*points.count);

    int indicies[WIDTH*HEIGHT] = {
        [0 ... WIDTH*HEIGHT-1] = -1
    };
    for (size_t y = 0; y < HEIGHT; ++y) {
        for (size_t x = 0; x < WIDTH; ++x) {
            size_t min_i = -1;
            int min = INT32_MAX;
            for (size_t i = 0; i < points.count; ++i) {
                int dx = points.items[i].x - x;
                int dy = points.items[i].y - y;
                int distance = dx*dx + dy*dy;
                if (min > distance) {
                    min = distance;
                    min_i = i;
                }
            }
            assert(min_i != -1);

            indicies[y * WIDTH + x] = min_i;
            sums[min_i].count += 1;
            switch (colour_space) {
                case CS_HSV: {
                    Vector3 hsv = ColorToHSV(GetImageColor(img, x, y));
                    sums[min_i].hsv.h += hsv.x;
                    sums[min_i].hsv.s += hsv.y;
                    sums[min_i].hsv.v += hsv.z;
                }; break;
                case CS_RGB: {
                    Color col = GetImageColor(img, x, y);
                    sums[min_i].rgb.r += (size_t)col.r;
                    sums[min_i].rgb.g += (size_t)col.g;
                    sums[min_i].rgb.b += (size_t)col.b;
                }; break;
                case CS_LEN: assert(0 && "Unreachable");
            }
        }
    }

    Color pixels[WIDTH*HEIGHT] = {0};
    for (size_t i = 0; i < WIDTH*HEIGHT; ++i) {
        Sum s = sums[indicies[i]];
        switch (colour_space) {
            case CS_HSV: {
                pixels[i] = ColorFromHSV(
                    s.hsv.h / s.count,
                    s.hsv.s / s.count,
                    s.hsv.v / s.count
                );
            } break;
            case CS_RGB: {
                pixels[i] = (Color) {
                    .r = s.rgb.r / s.count,
                    .g = s.rgb.g / s.count,
                    .b = s.rgb.b / s.count,
                    .a = 0xff,
                };
            } break;
            case CS_LEN: assert(0 && "Unreachable");
        }
    }
    UpdateTexture(texture, pixels);
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image>\n", argv[0]);
        return 1;
    }

    const char *file_path = argv[1];

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WIDTH, HEIGHT, "Voronoi Wallpaper");
    Image img = LoadImage(file_path);
    ImageResize(&img, WIDTH, HEIGHT);
    Texture2D texture = LoadTextureFromImage(img);
    Texture2D vtexture = LoadRenderTexture(WIDTH, HEIGHT).texture;

    bool show_points = false;
    bool show_texture = true;
    bool show_overlay = true;
    ColourSpace colour_space = CS_RGB;

    Vectors p = {0};
    fill_random(&p);
    voronoi(vtexture, p, img, colour_space);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x191919ff));

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            da_append(&p, GetMousePosition());
            if (show_texture) voronoi(vtexture, p, img, colour_space);
        } else if (IsKeyPressed(KEY_R)) {
            randomise(&p, 50);
            if (show_texture) voronoi(vtexture, p, img, colour_space);
        } else if (IsKeyPressed(KEY_SPACE)) {
            TraceLog(LOG_INFO, "Refilling random points");
            p.count = 0;
            fill_random(&p);
            if (show_texture) voronoi(vtexture, p, img, colour_space);
        } else if (IsKeyPressed(KEY_P)) {
            show_points ^= 1;
        } else if (IsKeyPressed(KEY_T)) {
            show_texture ^= 1;
            if (show_texture) voronoi(vtexture, p, img, colour_space);
        } else if (IsKeyPressed(KEY_S)) {
            colour_space = (colour_space + 1)%CS_LEN;
            if (show_texture) voronoi(vtexture, p, img, colour_space);
        } else if (IsKeyPressed(KEY_O)) {
            show_overlay ^= 1;
        }

        if (show_texture) {
            DrawTexture(vtexture, 0, 0, WHITE);
        }

        if (show_points) {
            for (size_t i = 0; i < p.count; ++i) {
                Vector2 *point = &p.items[i];
                DrawCircleV(*point, 3, WHITE);
            }
        }

        if (show_overlay) {
            DrawText(TextFormat("Colour Space: %s\n", cs_names[colour_space]), 0, 0, 30, WHITE);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
