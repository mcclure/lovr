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
	std::string run;
} pass;

struct {
	bool dead; // TODO: Some kind of reset protocol
	int channels;
	std::string run;
	Channel* send;
	Channel* recv;
	size_t bufferTrueLength;
	Blob *buffer;              // TODO: DON'T USE STATIC STORAGE, USE LOVRALLOC
} state;

extern "C" {

int EmptyAudio(int16_t *output, int frameCount) {
	memset(output, 0, frameCount*sizeof(int16_t));
	return 1;
}

int CrashAndReturnEmpty(Channel *send, std::string err, int16_t *output, int frameCount) {
	Variant variant; variant.type = TYPE_STRING; variant.value.string = strdup(err.c_str()); // l_thread_channel will dealloc
	uint64_t dummy;
	lovrChannelPush(send, &variant, -1, &dummy);
	return EmptyAudio(output, frameCount);
}

static const char *audioBlobName = "Audio thread output";

int RenderAudio(int16_t *output, unsigned long frameCount) {
	if (state.dead) return EmptyAudio(output, frameCount);
	if (pass.dirty) {
		bool reset = false;

		mtx_lock(&pass.lock);
		pass.dirty = false;
		reset = state.run != pass.run || state.channels != pass.channels;
		if (reset) { state.run = pass.run; state.channels = pass.channels; }
		mtx_unlock(&pass.lock);

		if (reset) {
			lovrRelease(Channel, state.send); state.send = NULL;
			lovrRelease(Channel, state.recv); state.recv = NULL;
			if (!state.run.empty()) {
				std::string send = state.run + "-up";
				std::string recv = state.run + "-dn";
				state.send = lovrThreadGetChannel(send.c_str()); lovrRetain(state.send); lovrChannelClear(state.send);
				state.recv = lovrThreadGetChannel(recv.c_str()); lovrRetain(state.recv); lovrChannelClear(state.recv);
			}
		}
	}
	if (!state.send) return EmptyAudio(output, frameCount);

	if (frameCount > state.bufferTrueLength) {
		lovrRelease(Blob, state.buffer);
		state.buffer = lovrBlobCreate(malloc(frameCount*sizeof(int16_t)), frameCount*sizeof(int16_t), audioBlobName);
		state.bufferTrueLength = frameCount;
	}
	void *bufferDataWas = state.buffer->data;
	state.buffer->size = frameCount*sizeof(int16_t);
	Variant variant; variant.type = TYPE_OBJECT;
	variant.value.object.pointer = state.buffer;
	variant.value.object.type = "Blob";
	variant.value.object.destructor = lovrBlobDestroy;
	uint64_t dummy;
	lovrChannelPush(state.send, &variant, -1, &dummy);
	bool popResult = lovrChannelPop(state.recv, &variant, 0.1);
	if (!popResult)
		return CrashAndReturnEmpty(state.recv, "Audio request timed out", output, frameCount);
	if (variant.type != TYPE_OBJECT || strcmp("Blob", variant.value.object.type))
		return CrashAndReturnEmpty(state.recv, "Audio request response of wrong type", output, frameCount);
	Blob *resultBlob = (Blob*)variant.value.object.pointer;
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
  const char *name = luaL_checkstring(L, 1);

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

	pass.channels = std::min(outInfo->maxOutputChannels, 2);
	PaStreamParameters outParam = {outDev, pass.channels,
			paInt16|paNonInterleaved, outInfo?outInfo->defaultLowOutputLatency:0, NULL};

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
  pass.run = name;

  mtx_unlock(&pass.lock);
  return 0;
}

static int l_audioStop(lua_State *L) {
  if (!pass.threadUp) return 0;
  lovrThrow("TODO");
  return 0;
}

const luaL_Reg audioLua[] = {
  { "start", l_audioStart },
  { "stop", l_audioStop },
  { NULL, NULL }
};

// Register Lua module-- not the same as gameInitFUnc
int luaopen_ext_audio(lua_State* L) {
  lua_newtable(L);
  luaL_register(L, NULL, audioLua);

  return 1;
}

}