#include "HardwareAdapter.h"
#include "RemoteAdapter.h"
#include <assert.h>
int main(){(void)Serial;assert(!hardware::pinsReady());assert(!remote::codesReady());assert(!hardware::initializeHardware());robot::RemoteState r;robot::Parameters p;remote::pollRemote(r,0,p);return 0;}
