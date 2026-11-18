/* PS4 system function stubs for OpenOrbis toolchain
 * These no-op implementations satisfy linker references from libSDL2.a
 * which was built with PS4 audio/video driver support.
 */

#include <stddef.h>

typedef unsigned int SceUID;
typedef int SceInt32;
typedef unsigned int SceUInt32;

/* Audio */
int sceAudioOutInit(void) { return 0; }
SceUID sceAudioOutOpen(int a, int b, int c, int d) { return -1; }
int sceAudioOutClose(SceUID uid) { return 0; }
int sceAudioOutOutput(SceUID uid, void *data) { return 0; }

/* Kernel */
void *sceKernelAllocateDirectMemory(size_t size, void *addr) { return (void *)0; }
SceUID sceKernelCreateEqueue(const char *name, int attr) { return -1; }
int sceKernelMapDirectMemory(void **addr, size_t size, int prot, int flags, SceUID memuid, long offset) { return -1; }
int sceKernelMunmap(void *addr, size_t size) { return 0; }
int sceKernelReleaseDirectMemory(void *addr) { return 0; }
int sceKernelWaitEqueue(SceUID equeue, void *data, unsigned int timeout) { return -1; }

/* Pad */
int scePadInit(int maxPorts) { return 0; }
SceUID scePadOpen(int port, int mode, void *opt) { return -1; }
int scePadClose(SceUID uid) { return 0; }
int scePadReadState(SceUID uid, void *state) { return -1; }
SceUID scePadGetHandle(int port) { return -1; }

/* User service */
int sceUserServiceInitialize(void) { return 0; }
int sceUserServiceGetInitialUser(SceUInt32 *userId) { return -1; }
int sceUserServiceGetLoginUserIdList(SceUInt32 *userIds, SceUInt32 maxUsers, SceUInt32 *outCount) { return -1; }

/* Video out */
int sceVideoOutOpen(int slot, void *opt) { return -1; }
int sceVideoOutClose(SceUID uid) { return 0; }
SceUID sceVideoOutRegisterBuffers(SceUID uid, void *buffers, int count, int width, int height, int pitch, int format) { return -1; }
int sceVideoOutUnregisterBuffers(SceUID uid) { return 0; }
int sceVideoOutSetFlipRate(SceUID uid, int rate) { return 0; }
int sceVideoOutSubmitFlip(SceUID uid, int bufferIndex, void *opt) { return -1; }
int sceVideoOutGetResolutionStatus(SceUID uid, void *status) { return -1; }
int sceVideoOutIsFlipPending(SceUID uid) { return 0; }
int sceVideoOutAddFlipEvent(SceUID uid, SceUID equeue, int data, int eventId) { return -1; }