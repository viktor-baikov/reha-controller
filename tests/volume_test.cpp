#include "../rehearsal_remote/Volume.hpp"
#include <assert.h>
int main() {
    remote::VolumePickup v;
    assert(!v.sample(0)); v.feedback(100);
    assert(!v.sample(0)); assert(!v.sample(60)); assert(v.sample(101));
    assert(v.sample(80)); v.feedback(80); assert(v.armed);
    v.feedback(30); assert(!v.armed); assert(!v.sample(79)); assert(v.sample(29));
    v.feedback(70); assert(!v.sample(20)); assert(v.sample(90));
    v.reset(); assert(!v.sample(90));
}
