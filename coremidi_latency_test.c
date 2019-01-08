#include "coremidi_latency_test.h"

#define NUM_SEND 500
#define MESSAGESIZE 3  
#define MAX_DIGIT 20

uint64_t start;
uint64_t end;
uint64_t elapsed;
uint64_t sumElapsed;

int processedCount;
int outputIndex;
int inputIndex;

OSStatus status;

MIDIClientRef midiclient = NULL;
MIDIPortRef midiout = NULL;
MIDIPortRef midiin = NULL;
pthread_mutex_t midiMutex;

static mach_timebase_info_data_t sTimebaseInfo;

const char *getDisplayName(MIDIObjectRef object) {
  CFStringRef name = nil;
  if (noErr != MIDIObjectGetStringProperty(object, 
                                           kMIDIPropertyDisplayName, &name)) {
    return NULL;
  }
  CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
  return CFStringGetCStringPtr(name, encodingMethod);
}

void listDevices() {
  ItemCount destCount = MIDIGetNumberOfDestinations();
  for (ItemCount i = 0 ; i < destCount ; ++i) {
    MIDIEndpointRef dest = MIDIGetDestination(i);
    if (dest != NULL) {
      printf("destination %d: %s\n", (int) i, getDisplayName(dest));
    }
  }

  ItemCount sourceCount = MIDIGetNumberOfSources();
  for (ItemCount i = 0 ; i < sourceCount ; ++i) {
    MIDIEndpointRef source = MIDIGetSource(i);
    if (source != NULL) {
      printf("source %d: %s\n", (int) i, getDisplayName(source));
    }
  }
}

void midiInCallback(const MIDIPacketList *packetList, 
                    void *readProcRefCon, 
                    void *srcConnRefCon) {

  end = mach_absolute_time();
  elapsed = end - start;
  sumElapsed += elapsed;

  if (processedCount == NUM_SEND - 1) {

    uint64_t avgElapsed = sumElapsed / NUM_SEND;
    uint64_t avgNano = avgElapsed * sTimebaseInfo.numer / 
                       sTimebaseInfo.denom;

    printf("avg_latency: %lld\n", avgNano);


    if (status = MIDIPortDispose(midiout)) {
      printf("Error trying to close MIDI output port: %d\n", status);
      printf("%s\n", GetMacOSStatusErrorString(status));
      exit(status);
    }

    midiout = NULL;

    pthread_mutex_destroy(&midiMutex);

    exit(0);
  }

  processedCount += 1;

  pthread_mutex_unlock(&midiMutex);
}

void runLatencyTest() {
  if ( sTimebaseInfo.denom == 0 ) {
      (void) mach_timebase_info(&sTimebaseInfo);
  }

  processedCount = 0;

  if (status = MIDIClientCreate(CFSTR("TeStInG"), NULL, NULL, &midiclient)) {
    printf("Error trying to create MIDI Client structure: %d\n", status);
    printf("%s\n", GetMacOSStatusErrorString(status));
    exit(status);
  }
   
  if (status = MIDIOutputPortCreate(midiclient, CFSTR("OuTpUt"), &midiout)) {
    printf("Error trying to create MIDI output port: %d\n", status);
    printf("%s\n", GetMacOSStatusErrorString(status));
    exit(status);
  }

  if (status = MIDIInputPortCreate(midiclient, CFSTR("InPuT"), midiInCallback, 
       NULL, &midiin)) {
    printf("Error trying to create MIDI output port: %d\n", status);
    printf("%s\n", GetMacOSStatusErrorString(status));
    exit(status);
  }

  MIDIEndpointRef dest;
  ItemCount destCount = MIDIGetNumberOfDestinations();
  for (ItemCount i = 0 ; i < destCount ; ++i) {
    dest = MIDIGetDestination(i);
    if ((int)i == outputIndex) {
      break;
    }
  }

  ItemCount sourceCount = MIDIGetNumberOfSources();
  for (ItemCount i = 0 ; i < sourceCount ; ++i) {
    MIDIEndpointRef source = MIDIGetSource(i);
    if ((int)i == inputIndex) {
      MIDIPortConnectSource(midiin, source, NULL);
    }
  }


  MIDITimeStamp timestamp = 0; // play immediately

  Byte bufferOn[1024];            
  Byte bufferOff[1024];

  MIDIPacketList *packetlistOn = (MIDIPacketList*)bufferOn;
  MIDIPacket *currentpacketOn = MIDIPacketListInit(packetlistOn);

  MIDIPacketList *packetlistOff = (MIDIPacketList*)bufferOff;
  MIDIPacket *currentpacketOff = MIDIPacketListInit(packetlistOff);

  Byte noteon[MESSAGESIZE] = {0x90, 60, 90};
  Byte noteoff[MESSAGESIZE] = {0x80, 60, 90};

  MIDIPacketListAdd(packetlistOn, 
                    sizeof(bufferOn), 
                    currentpacketOn, 
                    timestamp, 
                    MESSAGESIZE, 
                    noteon);
  
  MIDIPacketListAdd(packetlistOff, 
                    sizeof(bufferOff), 
                    currentpacketOff, 
                    timestamp, 
                    MESSAGESIZE, 
                    noteoff);

  MIDIPacketList *notePacketlists[2] = {packetlistOn, packetlistOff};

  for (int i = 0; i < NUM_SEND; i++) {
    MIDIPacketList *packetlist = notePacketlists[i % 2];
    pthread_mutex_lock(&midiMutex);
    start = mach_absolute_time();
    if (status = MIDISend(midiout, dest, packetlist)) {
      printf("Problem sending MIDI data on port %d\n", dest);
      printf("%s\n", GetMacOSStatusErrorString(status));
      exit(status);
    }
   
   }
}

int isNumber(char *s) {
  char safe[MAX_DIGIT];
  strncpy(safe, s, MAX_DIGIT - 2);
  safe[MAX_DIGIT-1] = '\0';

  for (int i = 0; safe[i]; i++) {
    if (isdigit(safe[i]) == 0) {
      return 0;
    }
  }
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc > 2 && isNumber(argv[1]) && isNumber(argv[2])) {
    pthread_mutex_init(&midiMutex, NULL);
    inputIndex = atoi(argv[1]);
    outputIndex = atoi(argv[2]);

    runLatencyTest();

    CFRunLoopRef runLoop;
    runLoop = CFRunLoopGetCurrent();
    CFRunLoopRun();
  } else {
    listDevices();
  }

  return 0;
}