# Porting MultiModule protocols to Deviation
This guide describes the process of porting a protocol implemented in the DIY-MultiModule project to Deviation.  This guide is a bit incomplete - it doesn't include protocol options or telemetry.  (and I didn't actually port Traxxas protocol)

## Introduction
Both MultiModule and Deviation protocol drivers are implemented in two main functions.  One function initializes the protocol.  The other is a scheduled callback that uses the radio chip to send packets to the receiver.  Packets are also received in protocols that support telemetry.

The following sections document the steps used to port the Traxxas protocol implementation from MultiModule to Deviation.  All the examples are from this port.

## Prep work
Copy the MultiModule protocol file to the Deviation protocol/ directory.  Rename the file with a .c extension.
```
cp DIY-Multiprotocol-TX-Module/Multiprotocol/TRAXXAS_cyrf6936.ino deviation/src/protocol/traxxas_cyrf6936.c
```

## Optional changes
These are mostly related to differences in coding style between the projects.  Most of them are skipped in the Traxxas port.
- Variable type declarations (e.g. uint16_t -> u16)
- Spaces in statements (e.g. if(x==1) -> if (x == 1))


## Required changes
### Add Deviation includes, remove MM include
Remove the existing `#include` line(s). Add the following lines immediately after the header comment.
```
#include <stdlib.h>
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "telemetry.h"
#include "config/model.h"
```

### Change global #if
Change `#if defined(TRAXXAS_CYRF6936_INO)`
to `#ifdef PROTO_HAS_CYRF6936`.  Adjust for chip used by protocol.

### Change PROGMEM declarations to static
Every variable declaration that includes the PROGMEM qualifier must be changed to a static variable.
For example change `const uint8_t PROGMEM` to `static const uint8_t`.

### Remove \_\_attribute\_\_((unused)) qualifier
Delete it everywhere.

### Add static declarations for MM globals
At this point it's helpful to start running make.  A lot of errors will result.  One big category will be undefined variables.  Take the names from the error messages and look up the variable in the Multiprotocol.ino file to get it's declaration.  Add the declarations as statics in the protocol file.  Some may be able to converted to locals in the functions where they are used as a memory optimization.  For example, added to traxxas_cyrf6936.c were
```
static u16 bind_counter;
static u8 packet_count;
static u8 phase;
static u8 len;
static u8 RX_num;
static u8 cyrfmfg_id[6];
static u8 packet[TRAXXAS_PACKET_SIZE];
```

### Add Deviation <protocol>_Cmds function
Copy this function from another protocol implementation file.  If the protocol being ported supports telemetry, copy from a protocol that has telemetry.  If the protocol being ported has protocol options, copy from a protocol that has protocol options.  For the most part the updates need to this function will be self-explanatory.  Change the protocol name, number of channels, binding method, etc.  Look at similar existing protocol implementations as most of the possibilities are implemented in at least one place.

Some of the PROTOCMD_* commands are needed in all protocols.  Again use existing protocols as a guide.

Change the initialization function calls to the MM protocol <protocol>_init function.  Add the parameter "bind" which replaces the IS_BIND_IN_PROGRESS flag in the init function.  Add the PROTOCOL_SetBindState() function to the binding code path (if not an autobind protocol).

### Bind messages
Replace `BIND_DONE` with `PROTOCOL_SetBindState(0)`.

### Fixup the initialization function
The MultiModule initialization function may use special memory access functions to read configuration memory.  This is not necessary in Deviation.

Add a "bind" parameter to the function and use it instead of the IS_BIND_IN_PROGRESS MM define if necessary.

If the RX_num global is used in the initialization function, replace it with an appropriately scaled Model.fixed_id.

### Fixup auxiliary functions
MultiModule has a global set of scaling functions used to map mixer outputs to protocol channel values.  Deviation defines these functions in each protocol.  Use one from an existing protocol or copy from MM.

MM may define functions for sending config values from PROGMEM.  Implement these as necessary if they don't already exist.  For example the `CYRF_PROGMEM_ConfigSOPCode()` function in the Traxxas protocol becomes `CYRF_ConfigSOPCode()` in Deviation.

### Protocol options
The Traxxas protocol does not have protocol options.  If needed use an existing protocol (e.g. FrskyX) as a guide.

### Telemetry
The Traxxas protocol does not have telemetry.  If needed use an existing protocol as a guide.

Telemetry in Deviation can be implemented independently for each protocol, but multiple protocols share a particular "parent" telemetry format (because it's easier than defining a new one).  For simple telemetry the TELEM_DSM format is fairly straightforward.  For example it is used in the Bayang protocol implementation.