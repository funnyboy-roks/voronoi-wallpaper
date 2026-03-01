#include <float.h>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define da_reserve(da, expected_capacity)                                                  \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = 256;                                                      \
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

Color colors[] = {
    LIGHTGRAY,
    YELLOW,
    VIOLET,
    GRAY,
    GOLD,
    ORANGE,
    PINK,
    DARKPURPLE,
    LIME,
    PURPLE,
    GREEN,
    SKYBLUE,
    DARKGRAY,
    BLUE,
    MAROON,
    BROWN,
    RED,
    BEIGE,
    DARKGREEN,
    DARKBROWN,
    DARKBLUE,
};
#define COLORS_LEN (sizeof(colors)/sizeof(colors[0]))

typedef struct {
    Vector2 *items;
    size_t count;
    size_t capacity;
} Vectors;

typedef struct {
    Vector2 a, b;
} Edge;

typedef struct {
    Edge *items;
    size_t count;
    size_t capacity;
} Edges;

int vec_cmp_xy(const void *a, const void *b) {
    Vector2 va = *(Vector2 *)a;
    Vector2 vb = *(Vector2 *)b;
    if (va.x == vb.x) {
        return va.y - vb.y;
    }
    return va.x - vb.x;
}

int vec_cmp_y(const void *a, const void *b) {
    Vector2 va = *(Vector2 *)a;
    Vector2 vb = *(Vector2 *)b;
    return vb.y - va.y;
}

Vector2 sort_base_v;
int edge_angle_sort(const void *a, const void *b) {
    Edge ea = *(Edge *)a;
    Edge eb = *(Edge *)b;

    Vector2 v1 = Vector2Subtract(ea.b, ea.a);
    Vector2 v2 = Vector2Subtract(eb.b, eb.a);
    float theta1 = Vector2Angle(v1, sort_base_v);
    float theta2 = Vector2Angle(v2, sort_base_v);
    if (theta1 < theta2) return -1;
    if (theta1 > theta2) return 1;
    return 0;
}


bool check_collision_lines_ignore_ends(Vector2 startPos1, Vector2 endPos1, Vector2 startPos2, Vector2 endPos2)
{
    bool collision = false;

    float rx = endPos1.x - startPos1.x;
    float ry = endPos1.y - startPos1.y;
    float sx = endPos2.x - startPos2.x;
    float sy = endPos2.y - startPos2.y;

    float div = rx*sy - ry*sx;

    if (fabsf(div) >= FLT_EPSILON)
    {
        float s12x = startPos2.x - startPos1.x;
        float s12y = startPos2.y - startPos1.y;

        float t = (s12x*sy - s12y*sx)/div;
        float u = (s12x*ry - s12y*rx)/div;

        if ((0.0f < t) && (t < 1.0f) && (0.0f < u) && (u < 1.0f))
        {
            collision = true;
        }
    }
    
    return collision;
}

bool in_circle(Vector2 a, Vector2 b, Vector2 c, Vector2 point) {
    a = Vector2Subtract(a, point);
    b = Vector2Subtract(b, point);
    c = Vector2Subtract(c, point);
    return (
        (a.x*a.x + a.y*a.y) * (b.x*c.y-c.x*b.y) -
        (b.x*b.x + b.y*b.y) * (a.x*c.y-c.x*a.y) +
        (c.x*c.x + c.y*c.y) * (a.x*b.y-b.x*a.y)
    ) > 0;
}

bool vector2_eq(Vector2 a, Vector2 b) {
    return fabs(a.x-b.x) < FLT_EPSILON && fabs(a.y-b.y) < FLT_EPSILON;
}

bool edge_eq(Edge a, Edge b) {
    return vector2_eq(a.a, b.a) && vector2_eq(a.b, b.b)
        || vector2_eq(a.b, b.a) && vector2_eq(a.a, b.b);
}

bool intersects_any(Edge edge, Edges left, Edges right) {
    for (size_t i = 0; i < left.count; ++i) {
        Edge e = left.items[i];
        if (check_collision_lines_ignore_ends(edge.a, edge.b, e.a, e.b))
            return true;
    }
    for (size_t i = 0; i < right.count; ++i) {
        Edge e = right.items[i];
        if (check_collision_lines_ignore_ends(edge.a, edge.b, e.a, e.b))
            return true;
    }
    return false;
}

void merge(
    Edges *left,
    Edges right,
    Vector2 *points,
    size_t points_count,
    size_t mid
) {
    size_t max_l = 0, max_r = mid;
    qsort(points,       mid,                sizeof(*points), vec_cmp_y);
    qsort(points + mid, points_count - mid, sizeof(*points), vec_cmp_y);
    size_t left_i = 0;
    size_t right_i = mid;
    // while (intersects_any((Edge) { points[left_i], points[right_i] }, *left, right)) {
    //     if (left_i == mid - 1 && right_i == points_count - 1) {
    //         assert(0 && "out of candidates for base LR-edge");
    //     }
    //     if (left_i == mid - 1) { right_i += 1; continue; }
    //     if (right_i == points_count - 1) { left_i += 1; continue; }

    //     Vector2 left_next = points[left_i + 1];
    //     Vector2 right_next = points[right_i + 1];

    //     float left_dy = fabsf(left_next.y - points[left_i].y);
    //     float right_dy = fabsf(right_next.y - points[right_i].y);
    //     if (left_dy < right_dy) left_i += 1;
    //     else right_i += 1;
    // }
    float min_l = INFINITY;
    float min_r = INFINITY;
    size_t min_li = 0;
    size_t min_ri = mid;
    bool set = false;
    for (size_t left_i = 0; left_i < mid; ++left_i) {
        for (size_t right_i = mid; right_i < points_count; ++right_i) {
            Edge edge = { points[left_i], points[right_i] };
            if (intersects_any(edge, *left, right)) {
                continue;
            }
            set = true;
            if (edge.a.y > min_l || edge.b.y > min_r) {
                min_li = left_i;
                min_ri = right_i;
            }
        }
    }
    if (!set) {
        printf("INVALID BASE!\n");
        return;
    }
    Edge base = { points[min_li], points[min_ri] };

    Edges connected = {0};
    for (size_t i = 0; i < right.count; ++i) {
        Edge e = right.items[i];
        if (vector2_eq(e.a, base.b)) {
            da_append(&connected, e);
        } else if (vector2_eq(e.b, base.b)) {
            e.b = e.a;
            e.a = base.b;
            da_append(&connected, e);
        }
    }
    assert(connected.count > 0);

    Vector2 base_v = sort_base_v = Vector2Subtract(base.a, base.b);
    qsort(connected.items, connected.count, sizeof(*connected.items), edge_angle_sort);

    Vector2 right_candidate = {0};
    bool has_right_candidate = false;
    for (size_t i = 0; i < connected.count; ++i) {
        Edge pc_edge = connected.items[i];
        Vector2 pc = pc_edge.b;
        float theta = Vector2Angle(base_v, Vector2Subtract(pc_edge.a, pc));
        if (theta > M_PI) {
            has_right_candidate = false;
            break;
        }
        if (i < connected.count - 1) {
            Edge npc_edge = connected.items[i + 1];
            Vector2 npc = npc_edge.b;

            if (in_circle(base.a, base.b, pc, npc)) {
                for (size_t j = 0; j < right.count; ++j) {
                    Edge e = right.items[j];
                    if (edge_eq(e, pc_edge)) {
                        Edge end = right.items[right.count - 1];
                        right.items[j] = end;
                        right.count -= 1;
                        break;
                    }
                }
                continue;
            }
        }
        right_candidate = pc;
        has_right_candidate = true;
    }

    connected.count = 0;
    for (size_t i = 0; i < left->count; ++i) {
        Edge e = left->items[i];
        if (vector2_eq(e.a, base.a)) {
            da_append(&connected, e);
        } else if (vector2_eq(e.b, base.a)) {
            e.b = e.a;
            e.a = base.a;
            da_append(&connected, e);
        }
    }
    base_v = sort_base_v = Vector2Subtract(base.b, base.a);
    qsort(connected.items, connected.count, sizeof(*connected.items), edge_angle_sort);

    Vector2 left_candidate = {0};
    bool has_left_candidate = false;
    for (size_t i = 0; i < connected.count; ++i) {
        Edge pc_edge = connected.items[i];
        Vector2 pc = pc_edge.b;
        float theta = Vector2Angle(base_v, Vector2Subtract(pc_edge.a, pc));
        if (theta > M_PI) {
            has_left_candidate = false;
            break;
        }
        if (i < connected.count - 1) {
            Edge npc_edge = connected.items[i + 1];
            Vector2 npc = npc_edge.b;

            if (in_circle(pc, base.a, base.b, npc)) {
                for (size_t j = 0; j < left->count; ++j) {
                    Edge e = left->items[j];
                    if (edge_eq(e, pc_edge)) {
                        Edge end = left->items[left->count - 1];
                        left->items[j] = end;
                        left->count -= 1;
                        break;
                    }
                }
                continue;
            }
        }
        left_candidate = pc;
        has_left_candidate = true;
    }

    if (has_right_candidate)
        DrawCircleV(right_candidate, 3, RED);
    if (has_left_candidate)
        DrawCircleV(left_candidate, 3, RED);

    if (has_left_candidate && has_right_candidate) {
        if (in_circle(left_candidate, base.a, base.b, right_candidate)) {
            has_left_candidate = false;
        } else {
            has_right_candidate = false;
        }
    }

    if (has_left_candidate) {
        da_append(left, ((Edge) {
            .a = left_candidate,
            .b = base.b,
        }));
        DrawLineV(left_candidate, base.b, RAYWHITE);
    } else if (has_right_candidate) {
        da_append(left, ((Edge) {
            .a = base.a,
            .b = right_candidate,
        }));
        DrawLineV(base.a, right_candidate, RAYWHITE);
    }

    da_append(left, base);
    da_reserve(left, right.count);
    memcpy(&left->items[left->count], right.items, right.count*sizeof(*right.items));

    DrawLineV(points[0], points[mid], RAYWHITE);
}


Edges divide(Vector2 *items, size_t count, size_t depth) {
    if (count <= 3) {
        Edges edges = {0};
        if (count == 3) {
            da_append(&edges, ((Edge) { items[0], items[1] }));
            da_append(&edges, ((Edge) { items[1], items[2] }));
            da_append(&edges, ((Edge) { items[2], items[0] }));
            DrawTriangleLines(items[0], items[1], items[2], RAYWHITE);
        } else if (count == 2) {
            da_append(&edges, ((Edge) { items[0], items[1] }));
            DrawLineV(items[0], items[1], RAYWHITE);
        } else {
            assert(0 && "Unreachable");
        }
        return edges;
    }

    size_t mid = count / 2;
    Edges left = divide(items, mid, depth + 1);
    Edges right = divide(items + mid, count - mid, depth + 1);
    merge(&left, right, items, count, mid);
    free(right.items);

    return left;
}

void triangulate(Vectors points) {
    qsort(points.items, points.count, sizeof(*points.items), vec_cmp_xy);
    Edges edges = divide(points.items, points.count, 0);

    free(edges.items);
}

int main(void) {
    InitWindow(1200, 900, "Delaunay Triangulation");

    Vectors points = {0};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x191919ff));

        if (IsKeyPressed(KEY_DELETE)) {
            points.count = 0;
        }

        for (size_t i = 0; i < points.count; ++i) {
            DrawCircleV(points.items[i], 5, RAYWHITE);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            da_append(&points, GetMousePosition());
        }

        Vector2 mouse_pos = GetMousePosition();
        DrawText(TextFormat("(%.0f, %.0f)", mouse_pos.x, mouse_pos.y), 5, 5, 36, RAYWHITE);

        if (points.count > 1) triangulate(points);


        EndDrawing();
    }
    return 0;
}
