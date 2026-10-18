#include "../rehearsal_remote/TextRows.hpp"
#include <assert.h>
#include <string.h>
int main() {
    remote::TextRows text;assert(!text.complete());
    text.cc(100,0,0); text.cc(101,3,1);
    text.cc(102,'R',2); text.cc(118,1,3); text.cc(102,'E',4); text.cc(102,'C',5);
    assert(text.rows[0][0] == 0);
    assert(text.cc(103,(3+'R'+'E'+'C')&127,6)); assert(strcmp(text.rows[0],"REC") == 0);
    assert(text.received[0] && !text.complete());
    text.cc(100,0,10); text.cc(101,1,11); text.cc(102,'X',12);
    assert(!text.cc(103,0,13)); assert(strcmp(text.rows[0],"REC") == 0);
    text.cc(100,1,20); text.cc(101,33,21); assert(text.row == 255);
    text.cc(100,1,30); text.cc(101,1,31); text.cc(102,127,32); assert(text.row == 255);
    text.cc(100,1,40); text.tick(1540); assert(text.row == 255);
    text.cc(100,1,2000); text.cc(101,0,2001); assert(text.cc(103,1,2002));
    assert(text.complete()); // an explicitly empty row is still valid metadata
    text.reset();assert(!text.complete() && !text.received[0] && !text.received[1]);
}
