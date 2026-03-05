#include "timing.h"

namespace Core {

static float s_delta_time = 0.0f;

float get_delta_time() {
    return s_delta_time;
}

void update_delta_time(float dt) {
    s_delta_time = dt;
}

void reset_delta_time() {
    s_delta_time = 0.0f;
}

}
