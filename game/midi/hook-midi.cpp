#include <sstream>
#include <unordered_map>
#include "RtMidi.h"
#include "platform.h"

static RtMidiOut midiProbeOut;
static RtMidiIn midiProbeIn;
using namespace std;

#define lovrLog printf // FIXME

// MIDI util

struct MidiDevice { // Notice: No destructor because never destroyed
  string name;
  int which;         // If multiple identical devices
  int port[2] = {};     // Out, in
  bool present[2] = {}; // Out, in
  RtMidiOut *rtOut;
  RtMidiIn *rtIn;
  MidiDevice(string _name, int _which) : name(_name), which(_which), rtOut(NULL), rtIn(NULL) {}
  RtMidi &rt(bool in) { // Ensure RtMidi object exists then 
    if (in) {
      if (!rtIn)
        rtIn = new RtMidiIn();
      return *rtIn;
    } else {
      if (!rtOut)
        rtOut = new RtMidiOut();
      return *rtOut;
    }
  }
  bool open(bool in) {   // Is open?
    if (in)
      return rtIn && rtIn->isPortOpen();
    else
      return rtOut && rtOut->isPortOpen();
  }
  void ensure(bool in) { // Make open
    if (!open(in))
      rt(in).openPort(port[in]);
  }
};

static vector<MidiDevice> devices;               // Device structures
static vector<int> inDevices, outDevices;        // Map of port->device array index (count always equal to existing devices)
unordered_map<string, vector<int>> deviceName;   // Map of (name, index)->device array index

// One step of correctMap. Fit a known-to-exist MIDI device into the devices tables
static void correctMapAddStep(unordered_map<string, int> &seen, int c, bool in) {
  string name = in ? midiProbeIn.getPortName(c) : midiProbeOut.getPortName(c);
  int which = seen[name]++; // If multiple identical devices
  int dc; // Now find index in devices list
  vector<int> &nameList = deviceName[name]; // All devices with this name
  if (nameList.size() > which) { // We have seen this device before
    dc = nameList[which];
  } else { // We have not seen this device before and must create it
    dc = devices.size();
    nameList.push_back(dc);
    devices.emplace_back(name, which);
  }
  MidiDevice &device = devices[dc]; // Set up device object
  device.port[in] = c;
  device.present[in] = true;
  (in ? inDevices : outDevices).push_back(dc);
}

// Run through connected devices and ensure nothing has changed
static bool correctMap() {
  bool reflow = false; // Did anything change?

  // Check number of outputs, names of outputs, number of inputs, names of inputs:
  int outc = midiProbeOut.getPortCount();
  if (outDevices.size() != outc) reflow = true;
  for(int c = 0; !reflow && c < outc; c++) reflow = devices[outDevices[c]].name != midiProbeOut.getPortName(c);
  int inc = midiProbeIn.getPortCount();
  if (inDevices.size() != inc) reflow = true;
  for(int c = 0; !reflow && c < inc; c++) reflow = devices[inDevices[c]].name != midiProbeIn.getPortName(c);

  if (reflow) { // Changed
    lovrLog("MIDI devices changed\n");

    // Clear and rebuild device number maps
    inDevices.clear(); outDevices.clear();
    for(MidiDevice &d : devices) d.present[0] = d.present[1] = false; // Nothing present until seen
    unordered_map<string, int> seen;
    for(int c = 0; reflow && c < outc; c++) correctMapAddStep(seen, c, false);
    seen.clear();
    for(int c = 0; reflow && c < inc; c++) correctMapAddStep(seen, c, true);

    for(int dc = 0; dc < devices.size(); dc++) { // Close leftover ports // FIXME count up to value
      MidiDevice &device = devices[dc];
      for(int in = 0; in < 1; in++) {
        if (!device.present[in] && device.open(in)) {
          device.rt(in).closePort();
        }
      }
    }
  }
  return reflow;
}

// Lua util

extern "C" {
#include "api/api.h"
#include "core/util.h"
}

static void setTableStack(lua_State *L, int idx, double number) {
  lua_pushnumber(L, number);
  lua_rawseti (L, -2, idx);
}

static void setTableStack(lua_State *L, int idx, string s) {
  lua_pushstring(L, s.c_str());
  lua_rawseti (L, -2, idx);
}

static void setTableStack(lua_State *L, int idx, bool b) {
  lua_pushboolean(L, b);
  lua_rawseti (L, -2, idx);
}

static void setTableStack(lua_State *L, string key, double number) {
  lua_pushstring(L, key.c_str());
  lua_pushnumber(L, number);
  lua_rawset (L, -3);
}

static void setTableStack(lua_State *L, string key, string s) {
  lua_pushstring(L, key.c_str());
  lua_pushstring(L, s.c_str());
  lua_rawset (L, -3);
}

static void setTableStack(lua_State *L, string key, bool b) {
  lua_pushstring(L, key.c_str());
  lua_pushboolean(L, b);
  lua_rawset (L, -3);
}

static double getTableNumberStack(lua_State *L, int idx, double opt) {
  lua_pushnumber(L, idx);
  lua_gettable(L, -2);
  double v = luaL_optnumber(L, -1, opt);
  lua_pop(L, 1);
  return v;
}

static double getTableNumberStack(lua_State *L, int idx) {
  lua_pushnumber(L, idx);
  lua_gettable(L, -2);
  double v = luaL_checknumber(L, -1);
  lua_pop(L, 1);
  return v;
}

static double getTableNumberStack(lua_State *L, string key) {
  lua_pushstring(L, key.c_str());
  lua_gettable(L, -2);
  double v = luaL_checknumber(L, -1);
  lua_pop(L, 1);
  return v;
}

static string getTableStack(lua_State *L, int idx) {
  lua_pushnumber(L, idx);
  lua_gettable(L, -2);
  string v = lua_tostring(L, -1);
  lua_pop(L, 1);
  return v;
}

static string getTableStack(lua_State *L, string key) {
  lua_pushstring(L, key.c_str());
  lua_gettable(L, -2);
  string v = lua_tostring(L, -1);
  lua_pop(L, 1);
  return v;
}

extern "C" {

// Debug

static int l_midiDebugPoke(lua_State *L) {
  int id = luaL_optnumber(L, 1, 1);
  switch(id) {
  case 1:{
    int outc = midiProbeOut.getPortCount();
    int inc = midiProbeIn.getPortCount();
    ostringstream str;
    str << "Out " << outc << endl;
    for(int c = 0; c < outc; c++) {
    	str << c << ": " << midiProbeOut.getPortName(c) << endl;
    }
    str << "In " << inc << endl;
    for(int c = 0; c < inc; c++) {
    	str << c << ": " << midiProbeIn.getPortName(c) << endl;
    }
    lua_pushstring(L, str.str().c_str());
    return 1;
  } break;
  case 2: {
    correctMap();
    ostringstream str;
    for(int c = 0; c < devices.size(); c++) {
      MidiDevice &d = devices[c];
      str << (c+1) << ": " << d.name << " (" << (d.which+1) << ") " << (d.present[0]?"Y":"N") << " " << (d.present[1]?"Y":"N") << endl;
    }
    lua_pushstring(L, str.str().c_str());
    return 1;
  } break;
  }
  return 0;
}

// Impl

static int l_midiDevicesUpdate(lua_State *L) {
  lua_pushboolean(L, correctMap());
  return 1;
}

static MidiDevice &getDevice(unsigned int id, bool in, bool mayAssert = true) {
  if (mayAssert)
    lovrAssert(devices.size() > id, "Bad device %d", id);
  MidiDevice &d = devices[id];
  d.ensure(in);
  return d;
}

static int l_midiDevices(lua_State *L) {
  bool suppressMap = lua_toboolean(L, 1);
  if (!suppressMap) correctMap();

  lua_newtable(L);

  for(int c = 0; c < devices.size(); c++) {
    MidiDevice &d = devices[c];
    lua_newtable(L);

    setTableStack(L, "name", d.name);
    setTableStack(L, "which", double(d.which+1));
    setTableStack(L, "hasOut", d.present[0]);
    setTableStack(L, "hasIn", d.present[1]);

    lua_rawseti (L, -2, c+1);
  }

  return 1;
}

static int l_midiSend(lua_State *L) {
  unsigned int id = luaL_checknumber(L, 1) - 1;
  lovrAssert(lua_type(L, 2) == LUA_TTABLE, "Sending non-table value to MIDI");
  lua_pushvalue(L, 2);

  MidiDevice &d = getDevice(id, false);

  vector<unsigned char> m;
  int i = 1;
  do {
    double v = getTableNumberStack(L, i, -1);
    if (v < 0)
      break;
    m.push_back(v);
  } while (i++);

  d.rtOut->sendMessage(&m);

  return 0;
}

static int l_midiRecv(lua_State *L) {
  unsigned int id = luaL_checknumber(L, 1) - 1;
  MidiDevice &d = getDevice(id, true);

  lua_newtable(L);
  for(int msgCount = 1;;msgCount++) {
    vector<unsigned char> msg;
    double stamp = d.rtIn->getMessage(&msg); // Toss timestamp?
    int msgSize = msg.size();
    if (!msgSize)
      break;
    lua_newtable(L);
    lua_pushnumber(L, stamp);
    lua_rawseti(L,-2,0);
    for(int c = 0; c < msgSize; c++) {
      lua_pushnumber(L, msg[c]);
      lua_rawseti(L,-2,c+1);
    }
    lua_rawseti(L,-2,msgCount);
  }

  return 1;
}

static int l_midiNote(lua_State *L) {
  unsigned int id = luaL_checknumber(L, 1) - 1;
  bool on = lua_toboolean(L, 2);
  int note = luaL_checknumber(L, 3);
  int vel = luaL_optnumber(L, 4, 127);
  
  MidiDevice &d = getDevice(id, false);

  vector<unsigned char> m;
  m.resize(3);
  m[0] = on ? 0x90 : 0x80;
  m[1] = note;
  m[2] = vel;
  d.rtOut->sendMessage(&m);

  return 0;
}

const luaL_Reg midiLua[] = {
  { "updateDevices", l_midiDevicesUpdate },
  { "devices", l_midiDevices },
  { "debugPoke", l_midiDebugPoke },
  { "send", l_midiSend },
  { "note", l_midiNote },
  { "recv", l_midiRecv },
  { NULL, NULL }
};

// Register Lua module-- not the same as gameInitFUnc
int luaopen_ext_midi(lua_State* L) {
  lua_newtable(L);
  luaL_register(L, NULL, midiLua);

  return 1;
}

}

// seq.cpp
// adapted from sketch_jul29b_seqwithtempo.ino 09a91542375

#include <math.h>

namespace seq {

#define FUNCS 26
#define STACKMAX 4
#define NOTEMAX 5
#define DEBUG 0
#define DEFAULTRATE 11025
#define DEFAULTHIGH 4096
//static int OCTAVE = 255/5;
static int ROOT = 60; // MIDI middle C

#define MONO (NOTEMAX == 1)

// Number: Play this note
// Parenthesis: Store a pattern, first letter is the label (must be uppercase letter)
// Uppercase letter: Play stored pattern
// +Number, -Number: Shift base note by semitones
// ++Number, --Number: Shift base note by octaves (or multiply/divide for non-notes)
// x: rest
// r: reset
// pNumber, p+Number, p++Number etc: Set pitch, do NOT play note
// vNumber, v+Number, v++Number etc: Set velocity
// tNumber, t+Number, t++Number etc: Set tempo (rate, high)
// hNumber, h+Number, h++Number etc: Set holdtime (high)

typedef enum {
  S_SCAN = 0,
  S_WANTTAG,
  S_WANTRPAREN,
  S_WANTFORK
} ParenState;

typedef enum {
  SI_SCAN = 0,
  SI_PITCH,
  SI_TEMPO,
  SI_VEL,
  SI_HOLD,
} InputState;

typedef enum {
  SM_SCAN = 0,
  SM_PLUS,
  SM_PLUS2,
  SM_MINUS,
  SM_MINUS2,
  SM_COUNT,
} ModState;

struct Frame {
  int at;
  int root;
  int pitch;
  int rate;
  int high;
  int vel;
  bool on;
  Frame() : at(0), root(0), pitch(0), rate(0), high(0), vel(127), on(false) {}
};

struct Interp;

struct InterpState {
  bool live;
  bool doomed; // Fork should terminate on next note
  int sp;
  ParenState state; ModState modState; InputState inputState;
  Frame stack[STACKMAX];
  InterpState() : live(false), sp(1), doomed(false) {
    resetState();
  }

  void reset();
  void resetState();
  void next(Interp &interp);
  void descend();
  bool invalid() { return sp >= STACKMAX; }
  Frame &frame() { return stack[sp]; }
  Frame &lastFrame() { return stack[sp-1]; }
  int &at() { return frame().at; }
  bool takeNumber(int n);
  void modNumber(int &target, int n);
};

struct Interp {
  // PARSER
  string song;
  InterpState notes[NOTEMAX];
  int func[FUNCS];

  // HACK: Stack pointer starts at 1 so r can work at the lowest level
  Interp(string _song, int _rate = DEFAULTRATE, int _high = DEFAULTHIGH) : song(_song) {
    memset(func, 0, sizeof(func));
    notes[0].stack[0].rate = notes[0].stack[1].rate = _rate;
    notes[0].stack[0].high = notes[0].stack[1].high = _high;
    notes[0].live = true;
  }
  void next(); // Iterate all
  int firstFree() {
    for(int c = 0; c < NOTEMAX; c++) {
      if (!notes[c].live) return c;
    }
    return -1;
  }
  void lives(bool *out) {
    for(int c = 0; c < NOTEMAX; c++)
      out[c] = notes[c].live;
  }
};

void InterpState::resetState() {
  state = S_SCAN;
  inputState = SI_SCAN;
  modState = SM_SCAN;
}

void InterpState::reset() {
  sp = 1; // In case of syntax error
  resetState(); // Ibid
  at() = 0;
}

static int modScale[SM_COUNT] = {0, 1, 12, -1, -12};

void InterpState::modNumber(int &target, int n) {
  switch (modState) {
    case SM_SCAN: target = n; break;
    case SM_PLUS: target += n; break;
    case SM_MINUS: target -= n; break;
    case SM_PLUS2: target *= n; break;
    case SM_MINUS2: target /= n; break;
    default:break;
  }
}

bool InterpState::takeNumber(int n) { // Return true if "done" with current note
  switch (inputState) {
    case SI_SCAN: case SI_PITCH:
      if (modState == SM_SCAN) {
        bool on = inputState == SI_SCAN;
        frame().on = on;
        frame().pitch = n;
        if (on) {
          if (sp == 0) doomed = true; // Are we a single-note fork? Then die here
          return true;
        }
      } else {
        frame().root += modScale[modState] * n;
      }
      break;
    case SI_TEMPO:
      modNumber(frame().rate, n);
      modNumber(frame().high, n);
      break;
    case SI_HOLD:
      modNumber(frame().high, n);
      break;
    case SI_VEL:
      modNumber(frame().vel, n);
      break;
  }
  resetState();
  return false;
}

void Interp::next() {
  bool play[NOTEMAX];
  for(int c = 0; c < NOTEMAX; c++) play[c] = notes[c].live;
  for(int c = 0; c < NOTEMAX; c++) {
    if (play[c])
      notes[c].next(*this);
  }
}

void InterpState::descend() {
  sp++;
  if (invalid()) return;
  frame() = lastFrame(); // Copy up
}

#define SERIAL_DEBUG 1
#if SERIAL_DEBUG
template<class T> void SERIAL_PRINT(T x) { cout << x << flush; }
#else
#define SERIAL_PRINT(...)
#endif

void InterpState::next(Interp &interp) {
  SERIAL_PRINT("\nNEXT");
  if (doomed) {
    live = doomed = false;
    return;
  }

  if (invalid()) return;
  bool isbuilding = false;
  int building = 0;
  bool done = false;
  // This loop iterates at the end, so saying "continue" (even inside a switch)
  // means "redo while considering this same character"
  while (!done) {
    int i = at();
    char ch = interp.song[i];
#if SERIAL_DEBUG
    SERIAL_PRINT("\nLoop");
    switch(state) { default:break; case S_WANTTAG: SERIAL_PRINT(" TAG?"); break; case S_WANTRPAREN: SERIAL_PRINT(" PAREN?"); }
    SERIAL_PRINT("\nmod ");
    SERIAL_PRINT(modState);
    SERIAL_PRINT("  root ");
    SERIAL_PRINT(frame().root);
    SERIAL_PRINT("\nstack ");
    SERIAL_PRINT(sp);
    SERIAL_PRINT("  at ");
    SERIAL_PRINT(i);
    SERIAL_PRINT("  ch ");
    SERIAL_PRINT(ch);
    SERIAL_PRINT("\n");
#endif
    if (ch == '\0') {
      bool done = false;
      if (isbuilding)
        done = takeNumber(building);
      reset();
      
      building = 0;
      isbuilding = false;
      
      if (done)
        return;
      else
        continue;
    } else switch (state) {
      case S_WANTTAG: // Notice; ()s can only be defined at top level
        if (isupper(ch))
          interp.func[ch-'A'] = i+1;
        state = S_WANTRPAREN;
        break;

      case S_WANTRPAREN:
        if (ch == ')')
          state = S_SCAN;
        break;

      case S_WANTFORK: {
        bool digit = isdigit(ch);
        bool brace = ch == '[';
        if (digit || isupper(ch) || brace) {
          int toIdx = interp.firstFree();
          if (toIdx >= 0) { // TODO?: Currently if we run out of free spaces we halt. No note stealing... yet.
            InterpState &to = interp.notes[toIdx];
            to.resetState(); // S_SCAN
            to.live = true;
            to.sp = 0;
            to.stack[0] = frame();
            SERIAL_PRINT("\nFORKING "); SERIAL_PRINT(toIdx);
            to.next(interp); // TODO?: This implies "DFS" rather than "BFS". Is this desired?
            SERIAL_PRINT("FORKED\n");
          }
          resetState();

          if (digit) { // Skip past digit
            do { at()++; ch = interp.song[at()]; } while (isdigit(ch));
            continue;
          }
          if (brace) { // Skip past []
            int depth = 0;
            do {
              if (ch == '[') depth++;
              if (ch == ']') depth--;
              at()++; ch = interp.song[at()]; // Increment before check
            } while (ch != '\0' && depth > 0);
            continue;
          }
        }
      } break;

      case S_SCAN:
        if (isdigit(ch)) {
          building *= 10;
          isbuilding = true;
          building += (ch - '0');
        } else {
          if (isbuilding) {
            if (takeNumber(building))
              return;
            building = 0;
            isbuilding = false;
          }

          if (isupper(ch)) {
            descend();
            if (invalid()) return;
            at() = interp.func[ch-'A'];
            continue;
          }

          switch (ch) {
            case 'x':
              frame().on = false;
              done = true;
              break;
            case 'r':
              frame().root = lastFrame().root;
              frame().rate = lastFrame().rate;
              frame().high = lastFrame().high;
              break;
            case 'p':
              inputState = SI_PITCH;
              break;
            case 't':
              inputState = SI_TEMPO;
              break;
            case 'h':
              inputState = SI_HOLD;
              break;
            case '+':
              modState = modState == SM_PLUS ? SM_PLUS2 : SM_PLUS;
              break;
            case '-':
              modState = modState == SM_MINUS ? SM_MINUS2 : SM_MINUS;
              break;
            case '&':
              state = S_WANTFORK;
              break;
            case '(':
              state = S_WANTTAG;
              break;
            case '[':
              descend();
              if (invalid()) return;
              break;
            case ']':
            case ')':
              sp--;
              cout << "SP AT " << sp << endl;
              if (sp < 1) { // Assume we're a fork and this is our EOL
                live = false;
                done = true;
              }
              if (ch == ']') // Save our IP progress to the outer stack
                at() = i;
              break;
            default: break; // Assume whitespace
          }
        }
        break;
    }
    at()++;
  }
}

} using namespace seq;

// hook-seq.cpp

#include <map>
#include "skategirl/util.h"
#include "lib/tinycthread/tinycthread.h"

struct ThreadOwner {
  thrd_t thread;
  mtx_t lock;
  cnd_t wake;
  bool running;
  bool everRan;
  bool dead;

  ThreadOwner(bool startAwake=false, int type=mtx_plain) : running(startAwake), dead(false) {
    mtx_init(&lock, mtx_plain);
    cnd_init(&wake);
    thrd_create(&thread, ThreadOwner::bounce, this); // Notice how unsafe startAwake is
  }
  virtual ~ThreadOwner() {
    mtx_destroy(&lock);
    cnd_destroy(&wake);
    thrd_detach(thread);
  }
  static int bounce(void *p) { // Internal use
    ThreadOwner *to = (ThreadOwner *)p;
    return to->run();
  }
  void run(bool on) {
    if (on && !running) {
      mtx_lock(&lock);
        running = on;
        cnd_signal(&wake);
      mtx_unlock(&lock);
    } else {
      running = on;
    }
  }
  void kill() { // Safely close thread from another thread
    mtx_lock(&lock);
      dead = true;
      cnd_signal(&wake);
    mtx_unlock(&lock);
  }
  virtual void destruct() {} // Override if needed
  virtual bool tick(bool timedOut, timespec &nextWake) = 0; // Override
  virtual int run() {
    bool timedOut = false, finished = false;
    while (1) {
      if (finished) { // If "dead" flag was set at end of last loop
        if (everRan)
          destruct();        // child destructor
        delete this;
        return thrd_success; // Set down
      }
      bool wantWake = false;
      timespec nextWake;
      if (running) {
        wantWake = tick(timedOut, nextWake); // Call child code
        everRan = true;
      }
      int waitResult = thrd_success;
      mtx_lock(&lock);
        if (dead)           // kill() was called off-thread
          finished = true;
        else if (wantWake)  // child wants a timed wakeup
          waitResult = cnd_timedwait(&wake, &lock, &nextWake);
        else                // child waiting for external wakeup
          cnd_wait(&wake, &lock);
      mtx_unlock(&lock);
      timedOut = waitResult == thrd_timedout;
    }
  }
};

timespec timespec_zero() { timespec ts; ts.tv_sec = 0; ts.tv_nsec = 0; return ts; }
timespec timespec_one()  { timespec ts; ts.tv_sec = 0; ts.tv_nsec = 1; return ts; }

timespec timespec_now() {
  timespec ts;
  timespec_get(&ts, TIME_UTC);
  return ts;
}

timespec float_timespec(double v) {
  double integer;
  double decimal = modf(v, &integer);
  timespec result;
  result.tv_sec = integer;
  result.tv_nsec = decimal * 1e9;
  return result;
}

bool timespec_eq(timespec a, timespec b)
  { return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec; }
bool timespec_lt(timespec a, timespec b)
  { return a.tv_sec < b.tv_sec
    || (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec); }
bool timespec_lte(timespec a, timespec b)
  { return a.tv_sec < b.tv_sec
    || (a.tv_sec == b.tv_sec && a.tv_nsec <= b.tv_nsec); }
bool timespec_gt(timespec a, timespec b)
  { return timespec_lte(b,a); }
bool timespec_gte(timespec a, timespec b)
  { return timespec_lt(b,a); }

timespec timespec_add(timespec a, timespec b) {
  timespec result;
  result.tv_sec = a.tv_sec + b.tv_sec;
  result.tv_nsec = a.tv_nsec + b.tv_nsec;
  if (result.tv_nsec > 1e9) {
    result.tv_nsec -= 1e9;
    result.tv_sec += 1;
  }
  return result;
}

timespec timespec_sub(timespec a, timespec b) {
  timespec result;
  result.tv_sec = a.tv_sec - b.tv_sec;
  result.tv_nsec = a.tv_nsec;
  if (a.tv_nsec < b.tv_nsec) {
    result.tv_nsec += 1e9;
    result.tv_sec -= 1;
  }
  result.tv_nsec -= b.tv_nsec;
  return result;
}

timespec timespec_min(timespec a, timespec b) {
  return timespec_lte(a, b) ? a : b;
}

timespec timespec_max(timespec a, timespec b) {
  return timespec_lte(b, a) ? a : b;
}

struct Seq : public ThreadOwner {
  Interp interp;      // Interpreter
  Frame noteFrame[NOTEMAX];         // Last read note
  bool down[NOTEMAX]; // True if we're on the "Down" part of a note (ie should not read)
  double speed;       // BPM analogue
  timespec expectedWake[NOTEMAX]; // Detect if we were woken up early
  int id;             // Which device is playing?

  int playingId; // internal use, current note device

  Seq(string _song, int _id = -1, float _speed = 1) : id(_id), speed(_speed), interp(_song), ThreadOwner(), playingId(-1), down{} {
    for(int c = 0; c < NOTEMAX; c++) expectedWake[c] = timespec_zero();
  }

  virtual bool tick(bool timedOut, timespec &nextWake) {
    timespec now = timespec_now();
    bool setNextWake = false;
    bool firstPass = true;
    bool done[NOTEMAX] = {};

    if (timespec_eq(now, timespec_zero())) // Can this ever even happen?
      now = timespec_one();
cout << "\nPLAYING" << endl;
    while (true) {
      bool didAnything = false;

      bool live[NOTEMAX]; interp.lives(live);

      for(int c = 0; c < NOTEMAX; c++) {
        InterpState &note = interp.notes[c];
        Frame &state = noteFrame[c];
if (live[c] and not done[c]) cout << "NOTE " << c << endl;
        if (done[c] || !live[c]) continue;
        didAnything = true;

        if (timespec_gte(now, expectedWake[c])) {
          int nextTicks = 0;
          bool wasDown = down[c];

          if (down[c]) {
            nextTicks = state.rate - state.high; // FIXME: What if nextTicks is negative?
            down[c] = false;
          } else {
            if (firstPass) // After first pass everything is forked; those get run next() for you.
              note.next(interp);
            state = note.frame();
            
            down[c] = note.live && state.on;
            if (note.live)
              nextTicks = down[c] ? state.high : state.rate;
            playingId = id;
          }

          if ((down[c] || wasDown) && playingId >= 0 && id < devices.size()) {
            MidiDevice &d = getDevice(playingId, false, false);
            int p = 60 + state.root + state.pitch;
            vector<unsigned char> m;
            m.resize(3);
            m[0] = !wasDown ? 0x90 : 0x80;
            m[1] = p;
            m[2] = note.live ? state.vel : 0;
            cout << "SENT " << (int)m[0] << " " << (int)m[1] << " " << (int)m[2] << endl;
            d.rtOut->sendMessage(&m);
            //lovrLog("SENT %d %d %s\n", (int)p, (int)state.vel, wasDown?"OF":"ON");
          }

          if (note.live) {
            timespec delta = float_timespec( nextTicks * speed / 44100.0 );
            expectedWake[c] = timespec_add( now, delta );
          }
          //lovrLog("HOW LONG TO NEXT: id %d ticks %d speed %lf time %d : %d\n", playingId, nextTicks, speed, (int)delta.tv_sec, (int)delta.tv_nsec);
        }

        if (note.live) {
          nextWake = !setNextWake ? expectedWake[c] : timespec_min(nextWake, expectedWake[c]);
          setNextWake = true;
          done[c] = true;
        }

        if (0) {
          timespec diff = timespec_sub(nextWake, now);
          lovrLog("NOTE %d: TIMED OUT %s: NOW WAIT %ld : %ld\n", c, timedOut?"Y":"N", (long)diff.tv_sec, (long)diff.tv_nsec);
        }
      }

      firstPass = false;
      if (!didAnything) // Loop until an entire cycle with no new notes
        break;
    }

    return setNextWake;
  }
};

static IdMap<Seq> seqs;
unsigned int seqGenerator;

template<class T>
static int createPtrStack(lua_State *L, map<unsigned int, T*> &m, unsigned int &generator, T *v) {
  unsigned int id = generator++;
  m[id] = v;
  lua_pushnumber(L, id);
  return 1;
}

extern "C" {

static int l_midiSeqNew(lua_State *L) {
  const char *str = luaL_optstring(L, 1, "");

  int deviceId = luaL_checknumber(L, 2) - 1;
  getDevice(deviceId, false); // ensure

  double speed = luaL_optnumber(L, 3, 1);

  Seq *seq = new Seq(str, deviceId, speed);

  return createPtrStack(L, seqs, seqGenerator, seq);
};

static int l_midiSeqDelete(lua_State *L) {
  unsigned int id = luaL_checknumber(L, 1);
  seqs.deleteOne(id);
  return 0;
}

static int l_midiSeqDeleteAll(lua_State *L) {
  seqs.deleteAll();
  return 0;
}

static int l_midiSeqPlay(lua_State *L) {
  unsigned int sid = luaL_checknumber(L, 1);
  lovrAssert(seqs.count(sid), "Bad sequence %d", sid);
  seqs[sid]->run(true);
  return 0;
}

static int l_midiSeqStop(lua_State *L) {
  unsigned int sid = luaL_checknumber(L, 1);
  lovrAssert(seqs.count(sid), "Bad sequence %d", sid);
  seqs[sid]->run(false);
  return 0;
}

static int l_midiSeqSetSpeed(lua_State *L) {
  unsigned int sid = luaL_checknumber(L, 1);
  lovrAssert(seqs.count(sid), "Bad sequence %d", sid);
  double speed = luaL_checknumber(L, 2);
  seqs[sid]->speed = speed;
  return 0;
}

static int l_midiSeqAt(lua_State *L) {
  unsigned int sid = luaL_checknumber(L, 1);
  lovrAssert(seqs.count(sid), "Bad sequence %d", sid);
  lua_pushnumber(L, seqs[sid]->interp.notes[0].at());
  return 1;
}

const luaL_Reg midiSeqLua[] = {
  { "new", l_midiSeqNew },
  { "delete", l_midiSeqDelete },
  { "deleteAll", l_midiSeqDeleteAll },
  { "play", l_midiSeqPlay },
  { "stop", l_midiSeqStop },
  { "setSpeed", l_midiSeqSetSpeed },
  { "at", l_midiSeqAt },
  { NULL, NULL }
};

// Register Lua module-- not the same as gameInitFUnc
int luaopen_ext_midi_seq(lua_State* L) {
  seqs.deleteAll();

  lua_newtable(L);
  luaL_register(L, NULL, midiSeqLua);

  return 1;
}

}