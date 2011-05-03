/* sound - 3D sound project
 *
 * Copyright (C) 2011 Trevor Dodds <@gmail.com trev.dodds>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* The Shoot-em-up Project - A 3D game (and engine/level editor) written
 * using C++, OpenGL/GLUT, OpenAL, for windows and linux.
 *
 * Copyright (C) 2005 Trevor Dodds <trev@primatedesign.co.uk> and Ben
 * Jones <sheepyl33t@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "soundmanager.h"

SoundManager::SoundManager()
{
  alGetError(); // clear error code
  /*cout << "getting list of sound devices" << endl;
  const ALCchar* temp = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  int i = 0;
  while (temp[i] != 0) {
    cout << temp[i] << endl;
    i += strlen(&temp[i]) + 1;
  }
  cout << "got list" << endl;
*/

  // doesn't work on linux - see alc_context.c
  //playDevice = alcOpenDevice(NULL); // open default device
  /*const ALCchar* temp = alcGetString(playDevice, ALC_DEVICE_SPECIFIER);
  int err = alGetError();
  if (err != AL_NO_ERROR) {
    cout << "OPENAL ERROR: " << endl;
    switch (err) {
      case AL_INVALID_NAME:
        cout << "invalid name." << endl;
        break;
      case AL_INVALID_ENUM:
        cout << "invalid enum." << endl;
        break;
      case AL_INVALID_VALUE:
        cout << "invalid value." << endl;
        break;
      case AL_INVALID_OPERATION:
        cout << "invalid operation." << endl;
        break;
      case AL_OUT_OF_MEMORY:
        cout << "out of memory." << endl;
        break;
      default:
        cout << err << endl;
        break;
    }
  }
  cout << "Device list:" << endl;
  while (*temp)
  {
    cout << temp << endl;
    temp += strlen(temp) + 1;
  }*/

  //char* devList = (char*) alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  //cout << "Error: " << alGetError() << endl;

    /*    while (*devList)  // I really hate this double null terminated list thing.
        {
            printf("  - %s\n", devList);
            devList += strlen(devList) + 1;
        } // while
*/

  playDevice = alcOpenDevice(NULL); // open default device
  if (playDevice)
  {
    playContext = alcCreateContext(playDevice, NULL);
    if (playContext)
    {
      alcMakeContextCurrent(playContext);
    }
  }else{
    cout << "Sound playback initialisation failed." << endl;
    int error = alGetError();
    cout << "Error num: " << error << endl;
  }

  // store twice as many samples as necessary - CAPTURE_SAMPLES * 2
  // so that while things are being processed the buffer is still capturing
  captureDevice = alcCaptureOpenDevice( NULL, CAPTURE_FREQ, AL_FORMAT_MONO16, CAPTURE_SAMPLES * 2);
  if (captureDevice == NULL) {
    cout << "Capture device initialisation failed." << endl;
    int error = alGetError();
    cout << "Error num: " << error << endl;
    if (error == ALC_INVALID_VALUE) cout << "invalid value" << endl;
    if (error == ALC_OUT_OF_MEMORY) cout << "out of memory" << endl;
  }

  capturedSamples = 0;
  captureBufWrite = 0; // start writing to first capture buffer
  captureBufRead = 0; // start reading first buffer

  for (int i = 0; i < CAPTURE_BUFFERS; i++) {
    captureBuf[i] = new ALbyte[CAPTURE_BUFFER_SIZE];
    captureBufIdx[i] = -1; // don't start sending captured data yet
  }

  receiveGain = 1;
  sourceGain = 0.5;
  for (int j = 0; j < MAX_STREAMING_SOURCES; j++) {
    receiveIdMap[j] = -1;
    receiveBufWrite[j] = 0;
    receiveQueueIdx[j] = 0;
    for (int i = 0; i < RECEIVE_BUFFERS; i++) {
      receiveBuf[j][i] = new ALbyte[CAPTURE_BUFFER_SIZE];
      receiveBufIdx[j][i] = 0; // ready to write to
      receiveQueue[j][i] = -1;
    }
  }

  minQueueSize = 2;
  
  // set listener position to zero
  listenerPos[0] = 0.0;
  listenerPos[1] = 0.0;
  listenerPos[2] = 0.0;

  for (int i = 0; i < MAX_SOURCES; i++) {
    position[i][0] = 0.0;
    position[i][1] = 0.0;
    position[i][2] = 0.0;

   sourceMap[i] = -1;
   sourceType[i] = TYPE_UNDETERMINED;
  }

  nextSource = 0;
  nextReceiveId = 0;
}

void SoundManager::resetListenerValues()
{
  setListenerOrientation(0.0, 0.0, 1.0, 0.0, 1.0, 0.0);
  setListenerVelocity(0.0, 0.0, 0.0);
  setListenerPosition(0.0, 0.0, 0.0);
}

ALboolean SoundManager::loadALData(char* filename, ALuint *buf)
{
  // Variables to load into.

  /*ALenum format;
  ALsizei size;
  ALvoid* data;
  ALsizei freq;
  ALboolean loop;*/

  // Load wav data into a buffer.

  // deprecated
  /*alutLoadWAVFile(filename, &format, &data, &size, &freq, &loop);
  alBufferData(*buf, format, data, size, freq);
  alutUnloadWAV(format, data, size, freq);*/
  //*buf = alutCreateBufferFromFile(filename);

  // Bind buffer with a source.
  //alSourcei (*src, AL_BUFFER,   *buf   );
  //alSourcef (*src, AL_PITCH,    1.0f     );
  //alSourcef (*src, AL_GAIN,     gain     );

  //alSourcefv(*src, AL_POSITION, SourcePos);
  //alSourcefv(*src, AL_VELOCITY, SourceVel);
  //alSourcei (*src, AL_LOOPING,  loop     );

  // Do another error check and return.
  if (alGetError() == AL_NO_ERROR)
    return AL_TRUE;

  cerr << "error with file: " << filename << endl;
  cerr << "...possibly file does not exist," << endl;
  cerr << "or wrong number of buffers/sources were generated" << endl;

  return AL_FALSE;

}

ALboolean SoundManager::loadSounds()
{
  filenames.push_back("sounds/argh.wav");
  filenames.push_back("sounds/argh2.wav");
  filenames.push_back("sounds/Boom-Public_d-160.wav");
  filenames.push_back("sounds/Explosio-The_Emi-7557.wav");
  filenames.push_back("sounds/Mega_Nuk-Sith_Mas-475.wav");
  filenames.push_back("sounds/2x_barre-The_Pain-1042.wav");
  filenames.push_back("sounds/Small_Gu-sss3d-1370.wav");

  if ((int) filenames.size() + CAPTURE_BUFFERS + RECEIVE_BUFFERS > MAX_SOUNDS) {
    cerr << "Error in sound.cpp: Number of filenames exceeds MAX_SOUNDS." << endl;
    exit(1);
  }

  // INIT
  /*cout << "obtaining device list" << endl;
  const ALCchar* devPtr = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  cout << "Error: " << alGetError() << endl;

  //if (devPtr[0] != NULL) {
    int i = 0;
    bool gotNull = false;
    while (1) {
      cout << devPtr[i];
      if (devPtr[i] == '\0') {
        if (gotNull) break;
        else {
          gotNull = true;
          cout << endl;
        }
      }else{
        gotNull = false;
      }
      i++;
    }
  //}
  cout << "got list" << endl;
     
  // attempting to make sound work on second card 
  const ALCchar* temp = (ALCchar*) "1"; //"EMU10K1";
  ALCdevice* Device = alcOpenDevice(temp); // select the "preferred device"
  if (Device) {
    cout << "ho!" << endl;
    ALCcontext* Context=alcCreateContext(Device,NULL);
    alcMakeContextCurrent(Context);
  }*/

  // create buffers
  alGenBuffers(MAX_SOUNDS, &buffer[0]);
  if (alGetError() != AL_NO_ERROR)
    return AL_FALSE;

  // create sources
  alGenSources(MAX_SOURCES, &source[0]);

  for (int i = 0; i < MAX_SOURCES; i++) {
    // the distance under which the volume of the source would normally drop by half
    // (before being influenced by rolloff factor or AL_MAX_DISTANCE)
    alSourcef(source[i], AL_REFERENCE_DISTANCE, 200);
//#ifdef CVE
    // don't want to hear loads of faint chatting noise
    alSourcef(source[i], AL_ROLLOFF_FACTOR, 6); // was 1.5
//#else
    //alSourcef(source[i], AL_ROLLOFF_FACTOR, 2); // was 1.5
//#endif
    alSourcef(source[i], AL_MIN_GAIN, 0); // default is 0, set to 1 when want to always hear
    //alSourcef(source[i], AL_MAX_DISTANCE, 100000); // attenuate (get quieter) up to this distance
  }

  if (alGetError() != AL_NO_ERROR)
    return AL_FALSE;

  bool returnValue = AL_TRUE;
  for (int i = 0; i < (int) filenames.size(); i++) {
    //if (loadALData((ALbyte*) filenames[i], &buffer[i], &source[i], (i == 1) ? 10.0 : 1.0) == AL_FALSE) returnValue = AL_FALSE;
    if (loadALData(filenames[i], &buffer[i]) == AL_FALSE) returnValue = AL_FALSE;
  }

  resetListenerValues();

  return returnValue;
}

void SoundManager::captureStart()
{
  capturedSamples = 0;
  alcCaptureStart(captureDevice);
}

void SoundManager::captureStop()
{
  alcCaptureStop(captureDevice);
  //cout << "Stopped capture device" << endl;
}

bool SoundManager::capture()
{
  //alcCaptureStart(captureDevice);

  alcGetIntegerv(captureDevice, ALC_CAPTURE_SAMPLES, sizeof (capturedSamples), &capturedSamples);
  if (capturedSamples > CAPTURE_SAMPLES - 1) { //CAPTURE_SAMPLES - 10) 
    //cout << "Storing captured stuff" << endl;
    //cout << "CapturedSamples: " << capturedSamples << endl;
    capturedSamples = CAPTURE_SAMPLES; // capture 1000 of them... the rest should be picked up next time
    alcCaptureSamples(captureDevice, captureBuf[captureBufWrite], capturedSamples);
    //alcCaptureSamples(captureDevice, captureBuf, CAPTURE_BUFFER_SIZE);
    //cout << "Stored." << endl;

    //alcCaptureStop(captureDevice);
    //cout << "Stopped device" << endl;

    //cout << "Strlen captureBuf: " << strlen(captureBuf) << endl;
    //for (int i = 0; i < 100; i++) cout << (int) captureBuf[i] << ",";
    //cout << endl;

    // make the buffer
    // TODO buffers 7-9 aren't needed
    //alBufferData(buffer[7+captureBufWrite], AL_FORMAT_MONO16, captureBuf[captureBufWrite], capturedSamples * 2, CAPTURE_FREQ);
    //                       TODO should be related to format (number of bytes)  ^^^^^^^^
    //cout << "Made the buffer. Buffer[" << (7+captureBufWrite) << "]: " << buffer[7+captureBufWrite] << endl;

    // attach it to a source
    //alSourcei (source[8], AL_BUFFER,   buffer[7+captureBufWrite]);
    //alSourcef (source[8], AL_PITCH,    1.0f     );
    //alSourcef (source[8], AL_GAIN,     500.0f     ); // > 100 doesn't seem to make much difference
    //cout << "attached it to a source" << endl;

    // if the data is captured faster than it is sent then it causes an overlap
    // i.e. the buffers get re-used before the data is sent and so some data is lost
    // adding more buffers may not prevent overlap if the data is continually sent at a slower
    // rate than the rate of capture
    if (captureBufIdx[captureBufWrite] > -1) cerr << "Overlap" << endl;
    captureBufIdx[captureBufWrite] = 0;
    captureBufWrite++;
    if (captureBufWrite > CAPTURE_BUFFERS - 1) captureBufWrite = 0;

    return false;
  }

  return true;

  
}

// If we're sending the same data again (for testing purposes) then need to reset this to 0
void SoundManager::setCaptureBufIdx(int i, int v)
{
  captureBufIdx[i] = v;
}

int SoundManager::getCaptureDataUnit(char* du)
{
  // this is set to -1 if buffer is not full and is set to 0 when buffer is filled
  if (captureBufIdx[captureBufRead] > -1) {
    //du = &captureBuf[captureBufIdx];
    for (int i = captureBufIdx[captureBufRead]; i < captureBufIdx[captureBufRead] + FLAG_AUDIO_SIZE; i++) {
      if (i > CAPTURE_BUFFER_SIZE - 1) {
        cout << "PADDING AUDIO" << endl;
        du[i - captureBufIdx[captureBufRead]] = (char) 0;
      }
      else du[i - captureBufIdx[captureBufRead]] = captureBuf[captureBufRead][i];
    }

    captureBufIdx[captureBufRead] += FLAG_AUDIO_SIZE;
    if (captureBufIdx[captureBufRead] > CAPTURE_BUFFER_SIZE - 1) {
      captureBufIdx[captureBufRead] = -1; // finished
      captureBufRead++; // read the next buffer
      if (captureBufRead > CAPTURE_BUFFERS - 1) captureBufRead = 0;
    }
    
    //return capturedAmount;
    return FLAG_AUDIO_SIZE;
  }else return -1;
}

int SoundManager::makeNewStreamingSource(int globalId)
{
  sourceMap[nextSource] = globalId;
  sourceType[nextSource] = TYPE_STREAMING;
  alSourceStop(source[nextSource]);

  //alSourcef(source[nextSource], AL_MAX_GAIN, 500.0);
  alSourcef(source[nextSource], AL_MIN_GAIN, 0.0);

  nextSource++;
  if (nextSource > MAX_SOURCES - 1) nextSource = 0;
  return (nextSource == 0) ? MAX_SOURCES - 1 : nextSource - 1;
}

int SoundManager::makeNewSource(int globalId, int buf)
{
  sourceMap[nextSource] = globalId;
  sourceType[nextSource] = TYPE_STATIC;

  alSourceStop(source[nextSource]);
  // bind buffer with source
  alSourcei (source[nextSource], AL_BUFFER,   buffer[buf]   );
  alSourcef (source[nextSource], AL_PITCH,    1.0f     );
  alSourcef (source[nextSource], AL_GAIN,     sourceGain);
  alSourcef(source[nextSource], AL_MIN_GAIN, 0.0);
  //alSourcef(source[nextSource], AL_MAX_GAIN, 1.0);

  nextSource++;
  if (nextSource > MAX_SOURCES - 1) nextSource = 0;
  return (nextSource == 0) ? MAX_SOURCES - 1 : nextSource - 1;
}

int SoundManager::getStreamingSource(int globalId)
{
  //int type;
  for (int i = 0; i < MAX_SOURCES; i++) {
    if (sourceMap[i] == globalId) {
      //alGetSourcei(source[i], AL_SOURCE_TYPE, &type);
      //if (type == AL_STREAMING)
      if (sourceType[i] == TYPE_STREAMING) {
        return i;
      }
    }
  }
  return makeNewStreamingSource(globalId);
}

int SoundManager::getSource(int globalId, int buf)
{
  int state = 0;
  //int type = 0;
  int count = 0;
  for (int i = 0; i < MAX_SOURCES; i++) {
    if (sourceMap[i] == globalId) {
      count++;
      //alGetSourcei(source[i], AL_SOURCE_TYPE, &type);
      // ignore it if it's streaming - it'll stop being streaming if it unqueues to zero buffers
      // with this manual system, it won't stop being streaming, but source can still be used if
      // makeNewSource or makeNewStreamingSource is called
      if (sourceType[i] != TYPE_STREAMING) {
        alGetSourcei(source[i], AL_SOURCE_STATE, &state);
        // if the source is still playing then make a new one
        if (state == AL_PLAYING) {
          // if have 2 sources already
          if (count > 1) alSourceStop(source[i]);
        }
        alGetSourcei(source[i], AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED || state == AL_INITIAL) {
          sourceType[i] = TYPE_STATIC;
          // bind buffer with source
          alSourcei (source[i], AL_BUFFER,   buffer[buf]   );
          alSourcef (source[i], AL_PITCH,    1.0f     );
          alSourcef (source[i], AL_GAIN,     sourceGain );
          return i;
        }
      }
    }
  }
  return makeNewSource(globalId, buf);
}

int SoundManager::makeNewReceiveId(int globalId)
{
  receiveIdMap[nextReceiveId] = globalId;
  
  nextReceiveId++;
  if (nextReceiveId > MAX_STREAMING_SOURCES - 1) nextReceiveId = 0;
  return (nextReceiveId == 0) ? MAX_STREAMING_SOURCES - 1 : nextReceiveId - 1;
}

int SoundManager::getReceiveId(int globalId)
{
  for (int i = 0; i < MAX_STREAMING_SOURCES; i++) {
    if (receiveIdMap[i] == globalId) return i;
  }
  return makeNewReceiveId(globalId);
}

void SoundManager::unqueuePlayedBuffers(int id, int sourceId)
{
  int numProcessed = 0;
  //cout << "Getting buffers processed" << endl;
  alGetSourcei(source[sourceId], AL_BUFFERS_PROCESSED, &numProcessed);
  //cout << "BUFFERS PROCESSED: " << numProcessed << endl;

  if (numProcessed > 0) {
#ifdef _LINUX_
    ALuint buffersToUnqueue[numProcessed];
#else
    ALuint buffersToUnqueue[10]; // note when changing number of buffers SEGFAULT could occur
#endif

    // example: if 3 buffers have been processed, then get first three off queue and add
    // them to be unqueued
    for (int i = 0; i < numProcessed; i++) {
      //cout << "Unqueuing buffer: " << (receiveQueue[i]) << endl;
      buffersToUnqueue[i] = buffer[10+10*id+receiveQueue[id][i]];
      receiveBufIdx[id][receiveQueue[id][i]] = 0; // ready to write to
      
      // not necessarily next buffer to unqueue since this can be called if play has stopped
      //buffersToUnqueue[i] = buffer[10+((receiveBufWrite + i) % RECEIVE_BUFFERS)];
      //receiveBufIdx[(receiveBufWrite + i) % RECEIVE_BUFFERS] = 0;
      //cout << "Unqueuing buffer: " << ((receiveBufWrite + i) % RECEIVE_BUFFERS) << endl;
    }

    // shift queue back down
    for (int i = 0; i < numProcessed; i++) {
      for (int i = 0; i < RECEIVE_BUFFERS - 1; i++) receiveQueue[id][i] = receiveQueue[id][i+1];
      receiveQueueIdx[id]--;
    }

    alSourceUnqueueBuffers(source[sourceId], numProcessed, &buffersToUnqueue[0]);
  }

  // not tested. Would need to set sourceType to TYPE_STREAMING after adding to queue
  //int numQueued = 0;
  //alGetSourcei(source[sourceId], AL_BUFFERS_QUEUED, &numQueued);
  //if (numQueued == 0) sourceType[sourceId] = TYPE_UNDETERMINED;
}

bool SoundManager::receiveAudio(int globalId, char* du, int length, float x, float y, float z, bool attenuate)
{
  int sourceId = getStreamingSource(globalId);
  int receiveId = getReceiveId(globalId);
  //cout << "sourceId: " << sourceId << endl;
  int idx = receiveBufIdx[receiveId][receiveBufWrite[receiveId]];
  
  if (idx == -1) { // check if buffer has been played and can be re-used
    //cout << "Buffer: " << receiveBufWrite << " has idx of -1 (full) so will check if it's been played (unqueue)" << endl;
    unqueuePlayedBuffers(receiveId, sourceId);
    idx = receiveBufIdx[receiveId][receiveBufWrite[receiveId]];
  }

  setSourcePosition(source[sourceId], x, y, z);

  if (idx > -1) {
    // TODO here if the data units don't divide equally into the buffer, some of the data will be lost
    for (int i = idx; i < idx + length && i < CAPTURE_BUFFER_SIZE; i++) receiveBuf[receiveId][receiveBufWrite[receiveId]][i] = du[i - idx];

    receiveBufIdx[receiveId][receiveBufWrite[receiveId]] += length;
    if (receiveBufIdx[receiveId][receiveBufWrite[receiveId]] > CAPTURE_BUFFER_SIZE - 1) {
      //cout << "Captured samples: " << capturedSamples << endl;
      //cout << "receiveBufWrite: " << receiveBufWrite << "... about to buffer" << endl;
      alBufferData(buffer[10+receiveId*10+receiveBufWrite[receiveId]], AL_FORMAT_MONO16, receiveBuf[receiveId][receiveBufWrite[receiveId]], CAPTURE_SAMPLES * 2, CAPTURE_FREQ);
      //                            TODO should relate this to number of bytes (i.e. MONO16 is 2) ^^^^^
      //cout << "Made the RECEIVED buffer. Buffer[" << (10+receiveBufWrite[receiveId]) << "]: " << buffer[10+receiveBufWrite[receiveId]] << endl;

      // attach it to a source
      //alSourcei (source[7], AL_BUFFER,   buffer[7]);
      alSourceQueueBuffers(source[sourceId], 1, &buffer[10+receiveId*10+receiveBufWrite[receiveId]]);
      alSourcef (source[sourceId], AL_PITCH,    1.0f     );
      //alSourcef(source[sourceId], AL_MAX_GAIN, 1000.0);
      alSourcef (source[sourceId], AL_GAIN,     receiveGain    ); // default is 1, MAX_GAIN default is 1
      //alSourcef(source[sourceId], AL_CONE_OUTER_GAIN, 1000.0);
      //alSourcef(source[sourceId], AL_CONE_INNER_ANGLE, 1000.0);
      //alSourcef(source[sourceId], AL_CONE_OUTER_ANGLE, 360.0);

      if (attenuate) alSourcef(source[sourceId], AL_MIN_GAIN, 0.0);
      else alSourcef(source[sourceId], AL_MIN_GAIN, 1.0);
      
      // queue stores relative buffer number i.e. 0-10 so can be used with receiveBufIdx
      receiveQueue[receiveId][receiveQueueIdx[receiveId]++] = receiveBufWrite[receiveId]; // note the buffer that has been queued
      //cout << "queued to a source" << endl;
      //cout << "Receive gain: " << receiveGain << endl; increasing didn't seem to make it louder

      int value = 0;
      alGetSourcei(source[sourceId], AL_SOURCE_STATE, &value);
      if (value == AL_INITIAL || value == AL_STOPPED) {
        unqueuePlayedBuffers(receiveId, sourceId);
        int numQueued = 0;
        alGetSourcei(source[sourceId], AL_BUFFERS_QUEUED, &numQueued);
        // this line removes the popping effect but introduces a larger occasional gap in the sound (it buffers ahead)
        // however this gap is only there if receive rate is constantly slower than play rate
        if (numQueued > minQueueSize) {
          //play(7, 0, x, y, z);
          alSourcei(source[sourceId], AL_SOURCE_RELATIVE, AL_FALSE);
          //setSourcePosition(source[sourceId], x, y, z); now done every time receiveAudio is called
          alSourcePlay(source[sourceId]);
        }
      }

      receiveBufIdx[receiveId][receiveBufWrite[receiveId]] = -1; // buffer full - don't write until played (set to 0 when played)
      receiveBufWrite[receiveId]++; // each source has 10 receive buffers
      if (receiveBufWrite[receiveId] > RECEIVE_BUFFERS - 1) receiveBufWrite[receiveId] = 0;
      
      //return true;
    }else cout << "ERROR: Not enough audio received ************************" << endl;
    return false;
  }else{
    cerr << "SoundManager::receiveAudio(): Error - receive buffer full." << endl;
    return true;
  }
}

void SoundManager::updateReceiver(int globalId, float x, float y, float z)
{
  for (int i = 0; i < MAX_SOURCES; i++) {
    if (sourceMap[i] == globalId) {
      setSourcePosition(source[i], x, y, z); // the sound appears to be too far away too quickly
    }
  }
  // map coords
  // the more you divide it by, the nearer it sounds
  //setSourcePosition(source[7], listenerPos[0] + (x - listenerPos[0]) / 50.0,
  //                             listenerPos[1] + (y - listenerPos[1]) / 50.0,
  //                             listenerPos[2] + (z - listenerPos[2]) / 50.0);
  //setSourcePosition(source[7], x, y, z); // the sound appears to be too far away too quickly
}

void SoundManager::setReceiveGain(float f)
{
  receiveGain = f;
}

float SoundManager::getReceiveGain()
{
  return receiveGain;
}

void SoundManager::setMinQueueSize(int q)
{
  minQueueSize = q;
}

int SoundManager::getMinQueueSize()
{
  return minQueueSize;
}

void SoundManager::setSourcePosition(ALuint source, ALfloat x, ALfloat y, ALfloat z)
{
  ALfloat sourcePosition[] = { x, y, z };
  alSourcefv(source, AL_POSITION, sourcePosition);
}

void SoundManager::setSourceDirection(ALuint source, ALfloat i, ALfloat j, ALfloat k)
{
  ALfloat sourceDirection[] = { i, j, k };
  alSourcefv(source, AL_DIRECTION, sourceDirection);
}

void SoundManager::setListenerPosition(ALfloat x, ALfloat y, ALfloat z)
{
  listenerPos[0] = x, listenerPos[1] = y, listenerPos[2] = z;
  alListenerfv(AL_POSITION, listenerPos);
}

void SoundManager::setListenerVelocity(ALfloat x, ALfloat y, ALfloat z)
{
  ALfloat vel[] = {x, y, z};
  alListenerfv(AL_VELOCITY, vel);
}

void SoundManager::setListenerOrientation(ALfloat x, ALfloat y, ALfloat z, ALfloat i, ALfloat j, ALfloat k)
{
  // Orientation of the listener. (first 3 elements are "at", second 3 are "up")
  ALfloat ori[] = {x, y, z, i, j, k};
  alListenerfv(AL_ORIENTATION, ori);
}

int SoundManager::play(int globalId, int buf, float x, float y, float z)
{
  //return 1; // TODO

  int s = getSource(globalId, buf);
  if (s > -1 && s < MAX_SOURCES) {
    // store position of source
    position[s][0] = x, position[s][1] = y, position[s][2] = z;
    // map source position to environment coords so it can be heard
    //setSourcePosition(source[s],
    //    listenerPos[0] + (x - listenerPos[0]) / 50.0,
    //    listenerPos[1] + (y - listenerPos[1]) / 50.0,
    //    listenerPos[2] + (z - listenerPos[2]) / 50.0);
    alSourcei(source[s], AL_SOURCE_RELATIVE, AL_FALSE);
    setSourcePosition(source[s], x, y, z);
    alSourcePlay(source[s]);

    return 1;
  }else{
    cerr << "SoundManager::play(): invalid sound number: " << s << endl;

    return 0;
  }
}

int SoundManager::playRelative(int globalId, int buf, float x, float y, float z)
{
  int s = getSource(globalId, buf);
  if (s > -1 && s < MAX_SOURCES) { // TODO
    // store offset of source
    position[s][0] = x, position[s][1] = y, position[s][2] = z;
    // set source to listener position plus offset
    //setSourcePosition(source[s], listenerPos[0] + x, listenerPos[1] + y, listenerPos[2] + z);
    alSourcei(source[s], AL_SOURCE_RELATIVE, AL_TRUE);
    setSourcePosition(source[s], x, y, z);
    alSourcePlay(source[s]);

    return 1;
  }else{
    cerr << "SoundManager::playRelative(): invalid sound number: " << s << endl;

    return 0;
  }
}

void SoundManager::updateListener(float x, float y, float z, float lx, float ly, float lz, float ux, float uy, float uz)
{
  //alListenerf(AL_GAIN, 2);
  setListenerPosition(x, y, z);
  setListenerOrientation(lx, ly, lz, ux, uy, uz);
  //setListenerVelocity();

  // update positions that are relative to listener

  // moving these doesn't seem to have an effect
  // i.e. once the shotgun sound has started playing if you
  // move then you hear it to your left or right

  // set source to listener position plus offset
  /*setSourcePosition(source[SOUND_PLAYER_HIT],
      listenerPos[0] + position[SOUND_PLAYER_HIT][0],
      listenerPos[1] + position[SOUND_PLAYER_HIT][1],
      listenerPos[2] + position[SOUND_PLAYER_HIT][2]);
  
  setSourcePosition(source[SOUND_SHOTGUN],
      listenerPos[0] + position[SOUND_SHOTGUN][0],
      listenerPos[1] + position[SOUND_SHOTGUN][1],
      listenerPos[2] + position[SOUND_SHOTGUN][2]);
  
  setSourcePosition(source[SOUND_MINIGUN],
      listenerPos[0] + position[SOUND_MINIGUN][0],
      listenerPos[1] + position[SOUND_MINIGUN][1],
      listenerPos[2] + position[SOUND_MINIGUN][2]);

  // remap sounds
  for (int i = 1; i < MAX_SOURCES; i++) {
    setSourcePosition(source[i],
        listenerPos[0] + (position[i][0] - listenerPos[0]) / 50.0,
        listenerPos[1] + (position[i][1] - listenerPos[1]) / 50.0,
        listenerPos[2] + (position[i][2] - listenerPos[2]) / 50.0);
  }*/
}

void SoundManager::killALData()
{
  for (int i = 0; i < CAPTURE_BUFFERS; i++) delete [] captureBuf[i];
  for (int j = 0; j < MAX_STREAMING_SOURCES; j++)
    for (int i = 0; i < RECEIVE_BUFFERS; i++)
      delete [] receiveBuf[j][i];

  //alDeleteBuffers((int) filenames.size() + CAPTURE_BUFFERS, &buffer[0]);
  alDeleteBuffers(MAX_SOUNDS, &buffer[0]);
  //alDeleteSources((int) filenames.size() + 1, &source[0]); // + 1 for capture source
  alDeleteSources(MAX_SOURCES, &source[0]); // + 1 for capture source
  //alutExit();
  if (playDevice) { // if failed to open then don't attempt to close cos it'll segfault
    //cout << "Closing play device" << endl;
    alcMakeContextCurrent(NULL);
    alcDestroyContext(playContext);
    alcCloseDevice(playDevice);
    cout << "Closed play device" << endl;
  }
  if (captureDevice != NULL) {
    if (alcCaptureCloseDevice(captureDevice)) {
      cout << "Capture closed ok" << endl;
    }else{
      cout << "Capture closed badly" << endl;
    }
  }
}

