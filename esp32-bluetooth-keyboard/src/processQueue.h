#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

// Pop one queued action and run it.
// A single key is tapped. A hold chord is pressed together.
// Returns true when a key was sent. Call from loop().
bool handleQueue(void);

#endif
