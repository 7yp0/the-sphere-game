#include "timing.h"
#include <chrono>

namespace Core {

static float s_delta_time = 0.0f;
using Clock = std::chrono::steady_clock;
static Clock::time_point s_start_time = Clock::now();

float get_delta_time() {
    return s_delta_time;
}

void update_delta_time(float dt) {
    s_delta_time = dt;
}

void reset_delta_time() {
    s_delta_time = 0.0f;
}


float get_time_seconds() {
    auto now = Clock::now();
    std::chrono::duration<float> elapsed = now - s_start_time;
    return elapsed.count();
}

}
