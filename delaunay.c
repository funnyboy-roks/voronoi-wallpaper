#include <stdio.h>
#include <raylib.h>
#include <assert.h>
#include <stdlib.h>
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

typedef struct {
    Vector2 *items;
    size_t count;
    size_t capacity;
} Vectors;

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

typedef struct {
    Vector2 a, b;
} Edge;

typedef struct {
    Edge *items;
    size_t count;
    size_t capacity;
} Edges;

bool intersects_any(Edge edge, Edges left, Edges right) {
    for (size_t i = 0; i < left.count; ++i) {
        Edge e = left.items[i];
        if (CheckCollisionLines(edge.a, edge.b, e.a, e.b, NULL))
            return true;
    }
    for (size_t i = 0; i < right.count; ++i) {
        Edge e = right.items[i];
        if (CheckCollisionLines(edge.a, edge.b, e.a, e.b, NULL))
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
    for (size_t left_i = 0; left_i < mid; ++left_i) {
        for (size_t right_i = mid; right_i < points_count; ++right_i) {
            Edge edge = { points[left_i], points[right_i] };
            if (intersects_any(edge, *left, right)) continue;
            if (edge.a.y < min_l || edge.b.y < min_r) {
                min_li = left_i;
                min_ri = right_i;
            }
        }
    }
    DrawLineV(points[0], points[mid], RAYWHITE);
}

Edges divide(Vector2 *items, size_t count) {
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
    Edges left = divide(items, mid);
    Edges right = divide(items + mid, count - mid);
    merge(&left, right, items, count, mid);

    return (Edges){0};
}

void triangulate(Vectors points) {
    qsort(points.items, points.count, sizeof(*points.items), vec_cmp_xy);
    divide(points.items, points.count);
}

int main(void) {
    InitWindow(1200, 900, "Delaunay Triangulation");

    Vectors points = {0};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x191919ff));

        for (size_t i = 0; i < points.count; ++i) {
            DrawCircleV(points.items[i], 5, RAYWHITE);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            da_append(&points, GetMousePosition());
        }

        if (points.count > 1) triangulate(points);


        EndDrawing();
    }
    return 0;
}
