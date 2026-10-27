// Port of src/engine/level/objects/cart.asm
#include "ported.h"

PORT_JUMP(CartStoppedFunc)
{
	JUMP(SetOneWayPlatformAction);
}
