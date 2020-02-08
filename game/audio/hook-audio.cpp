#include "portaudio.h"
extern "C" {
#include "api/api.h"
#include "core/util.h"
#include "core/hash.h"
#include "event/event.h"
#include "data/blob.h"
#include "thread/thread.h"
#include "thread/channel.h"
#include "core/ref.h"
}
#include <string>
#include "lib/tinycthread/tinycthread.h"

#define SAMPLERATE 48000
#define LOVR_DEBUG_AUDIOTAP 1
#define LOVR_DEBUG_FRAMESIZE 1

#ifdef LOVR_DEBUG_AUDIOTAP
// To get a record of what the audio callback is playing, define LOVR_DEBUG_AUDIOTAP,
// after running look in the lovr save directory for lovrDebugAudio.raw, and open as raw 32-bit floats
// Audacity (or Amadeus, on mac) can do this
#include <stdio.h>
static FILE* audiotapFile;
static bool audiotapWriting;
#endif

struct {
	bool threadUp;
	mtx_t lock;
	bool dirty;
	int channels;
	Thread *thread;
} pass;

struct {
	bool dead; // TODO: Some kind of reset protocol
	int channels;
	Thread* thread;
	lua_State *L;
	size_t bufferTrueLength;
	Blob *buffer;              // TODO: DON'T USE STATIC STORAGE, USE LOVRALLOC
#if LOVR_DEBUG_FRAMESIZE
	int frame;
#endif
} state;

extern "C" {

lua_State *threadSetup(Thread *thread);
void threadError(Thread *thread, lua_State*L);

int EmptyAudio(int16_t *output, int frameCount) {
	memset(output, 0, frameCount*sizeof(int16_t));
	return 1;
}

int CrashAndReturnEmpty(Thread *thread, std::string err, int16_t *output, int frameCount) {
	err = "Audio render thread: " + err;
	mtx_lock(&thread->lock); // DUPLICATE CODE WITH l_thread.c
	thread->error = (char *)malloc(err.length() + 1);
	if (thread->error) {
		memcpy(thread->error, err.c_str(), err.length() + 1);
		lovrEventPush((Event) {
			.type = EVENT_THREAD_ERROR,
			.data.thread = { thread, thread->error }
		});
	}
	thread->running = false;
	state.dead = true;
	mtx_unlock(&thread->lock);
	return EmptyAudio(output, frameCount);
}

static const char *audioBlobName = "Audio thread output";

int RenderAudio(int16_t *output, unsigned long frameCount) {
#if LOVR_DEBUG_FRAMESIZE
	if (state.frame == 4)
		lovrLog("Rendering audio, typical frame size %d\n", (int)frameCount);
	state.frame++;
#endif
	if (state.dead) return EmptyAudio(output, frameCount);
	if (pass.dirty) {
		bool reset = false;

		mtx_lock(&pass.lock);
		pass.dirty = false;
		reset = state.thread != pass.thread || state.channels != pass.channels;
		Thread *newThread;
		if (reset) { newThread = pass.thread; state.channels = pass.channels; }
		mtx_unlock(&pass.lock);

		if (reset) {
			if (state.thread) {
				// TODO shutdown thread
			}

			if (newThread) {
				state.L = threadSetup(newThread);
				state.thread = state.L ? newThread : NULL;
			}
		}
	}
	if (!state.thread) return EmptyAudio(output, frameCount);

	if (frameCount > state.bufferTrueLength) {
		lovrRelease(Blob, state.buffer);
		state.buffer = lovrBlobCreate(malloc(frameCount*sizeof(int16_t)), frameCount*sizeof(int16_t), audioBlobName);
		state.bufferTrueLength = frameCount;
	}
	void *bufferDataWas = state.buffer->data;
	state.buffer->size = frameCount*sizeof(int16_t);

	Blob *resultBlob = NULL;
	bool resultNil = false;
	int blobProgress = 0;
	lua_getglobal(state.L, "lovr");
	if (!lua_isnoneornil(state.L, -1)) {
		blobProgress++;
		lua_getfield(state.L, -1, "audio");
		if (!lua_isnoneornil(state.L, -1)) {
			blobProgress++;
			Variant variant; variant.type = TYPE_OBJECT;
			variant.value.object.pointer = state.buffer;
			variant.value.object.type = "Blob";
			variant.value.object.destructor = lovrBlobDestroy;

			luax_pushvariant(state.L, &variant);
			int result = lua_pcall (state.L, 1, 1, 0);

			if (result) {
				// Failure
				threadError(state.thread, state.L);
				state.dead = true;
			} else {
				blobProgress++;
				resultNil = lua_isnil(state.L, -1);
				if (!resultNil) {
					resultBlob = luax_totype(state.L, -1, Blob);
					lovrRetain(resultBlob);
				}
				lua_settop(state.L, 0);
			}
		}
	}
	if (resultNil)
		return EmptyAudio(output, frameCount);
	if (!resultBlob) {
		switch (blobProgress) {
			case 0:
				return CrashAndReturnEmpty(state.thread, "No lovr object in audio thread global scope", output, frameCount);
			case 1:
				return CrashAndReturnEmpty(state.thread, "No lovr.audio in audio thread", output, frameCount);
			case 2:
				return EmptyAudio(output, frameCount);
			default:
				return CrashAndReturnEmpty(state.thread, "lovr.audio returned wrong type", output, frameCount);
		}
	}
	int resultSize = std::min(resultBlob->size/sizeof(int16_t), frameCount);
	memcpy(output, resultBlob->data, resultSize*sizeof(int16_t));
	if (resultBlob != state.buffer) {
		lovrRelease(Blob, state.buffer);
		state.buffer = resultBlob;
	}
	if (bufferDataWas != state.buffer->data)
		state.bufferTrueLength = resultSize;

	if (resultSize < frameCount)
		memset(output + resultSize, 0, (frameCount-resultSize)*sizeof(int16_t));
	return 0;
}

#if 1
int PaRenderAudio(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags, void *module)
{
	int16_t **buffers = (int16_t **)output;
	int result = RenderAudio(buffers[0], frameCount);

#ifdef LOVR_DEBUG_AUDIOTAP
  	if (audiotapWriting)
    	fwrite(buffers[0], sizeof(uint16_t), frameCount, audiotapFile);
#endif

	for(int c = 1; c < state.channels; c++) {
		memcpy(buffers[c], buffers[0], frameCount*sizeof(int16_t));
	}
	return result ? paAbort : paContinue;
}
#endif

static int l_audioStart(lua_State *L) {
  Thread* thread = luax_checktype(L, 1, Thread);
  lovrAssert(!lovrThreadIsRunning(thread), "Thread for audio is already started");

  thread->argumentCount = MIN(MAX_THREAD_ARGUMENTS, lua_gettop(L) - 1);
  for (size_t i = 0; i < thread->argumentCount; i++) {
    luax_checkvariant(L, 2 + i, &thread->arguments[i]);
  }

  if (pass.threadUp) {
  	mtx_lock(&pass.lock);
  } else {
  	mtx_init(&pass.lock, mtx_plain);
#if 1
#ifdef LOVR_DEBUG_AUDIOTAP
  	audiotapFile = fopen("/tmp/lovrDebugAudio.raw", "w");
  	audiotapWriting = audiotapFile;
#endif
  	// Init portaudio
	PaError err = Pa_Initialize();
	if (err) lovrThrow("Error initializing PortAudio: %s\n", Pa_GetErrorText(err));

	int defaultApiIdx = Pa_GetDefaultHostApi();
	const PaHostApiInfo *api = Pa_GetHostApiInfo(defaultApiIdx);
	PaDeviceIndex outDev = Pa_GetDefaultOutputDevice();
	const PaDeviceInfo *outInfo = Pa_GetDeviceInfo(outDev);
lovrLog("DEFAULT LOW LATENCY: %d\n", (int)(outInfo?outInfo->defaultLowOutputLatency:0));
	pass.channels = std::min(outInfo->maxOutputChannels, 2);
	PaStreamParameters outParam = {outDev, pass.channels,
			paInt16|paNonInterleaved, std::max<PaTime>(512, outInfo?outInfo->defaultLowOutputLatency:0), NULL};

	mtx_lock(&pass.lock);

	PaStream *stream;
	err = Pa_OpenStream(&stream, NULL, &outParam, SAMPLERATE,
				paFramesPerBufferUnspecified, paNoFlag, &PaRenderAudio, NULL);
	if (err) {
		mtx_unlock(&pass.lock);
		lovrThrow("Error opening PortAudio stream: %s\n", Pa_GetErrorText(err));
	}

	err = Pa_StartStream(stream);
	if (err) {
		mtx_unlock(&pass.lock);
		lovrThrow("Error starting PortAudio stream: %s\n", Pa_GetErrorText(err));
	}

	pass.threadUp = true;
#endif
  }

  pass.dirty = true;
  pass.thread = thread;

  mtx_unlock(&pass.lock);
  return 0;
}

static int l_audioStop(lua_State *L) {
  if (!pass.threadUp) return 0;
  lovrThrow("TODO");
  return 0;
}

static int l_blobCopy(lua_State *L) {
  Blob* a = luax_checktype(L, 1, Blob);
  Blob* b = luax_checktype(L, 2, Blob);
  int len = MIN(a->size, b->size);
  memcpy(a->data, b->data, len);
  return 0;
}

const luaL_Reg audioLua[] = {
  { "start", l_audioStart },
  { "stop", l_audioStop },

  { "blobCopy", l_blobCopy },
  { NULL, NULL }
};

// Register Lua module-- not the same as gameInitFUnc
int luaopen_ext_audio(lua_State* L) {
  lua_newtable(L);
  luaL_register(L, NULL, audioLua);

  return 1;
}

}