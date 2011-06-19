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

#include "ogg.h"

// Can load file into memory all at once, or play from a file, depending which open method is called

/************************************************************************************************************************
  The following function are the vorbis callback functions.
  As their names suggest, they are expected to work in exactly the
  same way as normal c io functions (fread, fclose etc.).
  It's up to us to return the information that the libs need to parse
  the file from memory
************************************************************************************************************************/
//---------------------------------------------------------------------------------
// Function  : VorbisRead
// Purpose  : Callback for the Vorbis read function
// Info    : 
//---------------------------------------------------------------------------------
size_t VorbisRead(void *ptr      /* ptr to the data that the vorbis files need*/, 
          size_t byteSize  /* how big a byte is*/, 
          size_t sizeToRead /* How much we can read*/, 
          void *datasource  /* this is a pointer to the data we passed into ov_open_callbacks (our SOggFile struct*/)
{
  size_t        spaceToEOF;      // How much more we can read till we hit the EOF marker
  size_t        actualSizeToRead;  // How much data we are actually going to read from memory
  SOggFile*      vorbisData;      // Our vorbis data, for the typecast

  // Get the data in the right format
  vorbisData = (SOggFile*)datasource;

  // Calculate how much we need to read.  This can be sizeToRead*byteSize or less depending on how near the EOF marker we are
  spaceToEOF = vorbisData->dataSize - vorbisData->dataRead; // TODO do these take into account byteSize?
  if ((sizeToRead*byteSize) < spaceToEOF)
    actualSizeToRead = (sizeToRead*byteSize);
  else
    actualSizeToRead = spaceToEOF;  
  
  // A simple copy of the data from memory to the datastruct that the vorbis libs will use
  if (actualSizeToRead)
  {
    // Copy the data from the start of the file PLUS how much we have already read in
    memcpy(ptr, (char*)vorbisData->dataPtr + vorbisData->dataRead, actualSizeToRead);
    // Increase by how much we have read by
    vorbisData->dataRead += (actualSizeToRead);
  }

  // Return how much we read (in the same way fread would)
  return actualSizeToRead;
}

//---------------------------------------------------------------------------------
// Function  : VorbisSeek
// Purpose  : Callback for the Vorbis seek function
// Info    : 
//---------------------------------------------------------------------------------
int VorbisSeek(void *datasource    /*this is a pointer to the data we passed into ov_open_callbacks (our SOggFile struct*/, 
         ogg_int64_t offset  /*offset from the point we wish to seek to*/, 
         int whence      /*where we want to seek to*/)
{
  size_t        spaceToEOF;    // How much more we can read till we hit the EOF marker
  ogg_int64_t      actualOffset;  // How much we can actually offset it by
  SOggFile*      vorbisData;    // The data we passed in (for the typecast)

  // Get the data in the right format
  vorbisData = (SOggFile*)datasource;

  // Goto where we wish to seek to
  switch (whence)
  {
  case SEEK_SET: // Seek to the start of the data file
    // Make sure we are not going to the end of the file
    if (vorbisData->dataSize >= offset)
      actualOffset = offset;
    else
      actualOffset = vorbisData->dataSize;
    // Set where we now are
    vorbisData->dataRead = (int)actualOffset;
    break;
  case SEEK_CUR: // Seek from where we are
    // Make sure we dont go past the end
    spaceToEOF = vorbisData->dataSize - vorbisData->dataRead;
    if (offset < spaceToEOF)
      actualOffset = (offset);
    else
      actualOffset = spaceToEOF;  
    // Seek from our currrent location
    vorbisData->dataRead += actualOffset;
    break;
  case SEEK_END: // Seek from the end of the file
    vorbisData->dataRead = vorbisData->dataSize+1;
    break;
  default:
    printf("*** ERROR *** Unknown seek command in VorbisSeek\n");
    break;
  };

  return 0;
}

//---------------------------------------------------------------------------------
// Function  : VorbisClose
// Purpose  : Callback for the Vorbis close function
// Info    : 
//---------------------------------------------------------------------------------
int VorbisClose(void *datasource /*this is a pointer to the data we passed into ov_open_callbacks (our SOggFile struct*/)
{
  // This file is called when we call ov_close.  If we wanted, we could free our memory here, but
  // in this case, we will free the memory in the main body of the program, so dont do anything
  return 1;
}

//---------------------------------------------------------------------------------
// Function  : VorbisTell
// Purpose  : Classback for the Vorbis tell function
// Info    : 
//---------------------------------------------------------------------------------
long VorbisTell(void *datasource /*this is a pointer to the data we passed into ov_open_callbacks (our SOggFile struct*/)
{
  SOggFile*  vorbisData;

  // Get the data in the right format
  vorbisData = (SOggFile*)datasource;

  // We just want to tell the vorbis libs how much we have read so far
  return vorbisData->dataRead;
}
/************************************************************************************************************************
  End of Vorbis callback functions
************************************************************************************************************************/

void ogg_stream::open_inmemory(string path)
{


  /************************************************************************************************************************
    Heres a total bodge, just to get the file into memory.  Normally, the file would have been loaded into memory
    for a specific reason e.g. loading it from a pak file or similar.  I just want to get the file into memory for
    the sake of the tutorial.
  ************************************************************************************************************************/

  FILE*   tempOggFile;
  int    sizeOfFile; 
  //char  tempChar;
  //int    tempArray;

    if(!(tempOggFile = fopen(path.c_str(), "rb")))
        throw string("Could not open Ogg file.");

  // Find out how big the file is
  // awful dodgy code this - why not just seek to end?
  // That way you actually get the correct size (without the extra byte!)
  /*sizeOfFile = 0;
  while (!feof(tempOggFile))
  {
    tempChar = getc(tempOggFile);
    sizeOfFile++;
  }*/
  fseek(tempOggFile, 0, SEEK_END);
  sizeOfFile = ftell(tempOggFile);
  //cout << "sizeoffile: " << sizeOfFile << endl;

  // Save the data into memory
  oggMemoryFile.dataPtr = new char[sizeOfFile];
  rewind(tempOggFile);
  // not this again ;-)
  /*tempArray = 0;
  while (!feof(tempOggFile))
  {
    oggMemoryFile.dataPtr[tempArray] = getc(tempOggFile);
    tempArray++;
  }*/
  fread(oggMemoryFile.dataPtr, 1, sizeOfFile, tempOggFile);

  // Close the ogg file
  fclose(tempOggFile);

  // Save the data in the ogg memory file because we need this when we are actually reading in the data
  // We havnt read anything yet
  oggMemoryFile.dataRead = 0;
  // Save the size so we know how much we need to read
  oggMemoryFile.dataSize = sizeOfFile;  

  /************************************************************************************************************************ 
    End of nasty 'just stick it in memory' bodge...
  ************************************************************************************************************************/



  // This is really the only thing that is different from the original lesson 8 file...

  // Now we have our file in memory (how ever it got there!), we need to let the vorbis libs know how to read it
  // To do this, we provide callback functions that enable us to do the reading.  the Vorbis libs just want the result
  // of the read.  They dont actually do it themselves
  // Save the function pointersof our read files...
  vorbisCallbacks.read_func = VorbisRead;
  vorbisCallbacks.close_func = VorbisClose;
  vorbisCallbacks.seek_func = VorbisSeek;
  vorbisCallbacks.tell_func = VorbisTell;

  // Open the file from memory.  We need to pass it a pointer to our data (in this case our SOggFile structure),
  // a pointer to our ogg stream (which the vorbis libs will fill up for us), and our callbacks
  if (ov_open_callbacks(&oggMemoryFile, &oggStream, NULL, 0, vorbisCallbacks) != 0)
    throw string("Could not read Ogg file from memory");



  /************************************************************************************************************************
    From now on, the code is exactly the same as in Jesse Maurais's lesson 8
  ************************************************************************************************************************/
  initSource();
}



ogg_stream::ogg_stream()
{
  // clear error code
  alGetError();
  bufferIdx = 0;

  sourceInitialised = false;

  sourceId = -1;

  x = 0, y = 0, z = 0;
  speedX = 0, speedY = 0, speedZ = 0;
  paused = true;
  moving = false;
  homing = false;
  targX = 0, targY = 0, targZ = -50; // land just in front of listener
  friction = true;

  pitch = 1.0; // binaural test

  //alSpeedOfSound(100000); // default 343.3
  //check("Speed");

  /*playDevice = alcOpenDevice(NULL); // open default device
  if (playDevice)
  {
    playContext = alcCreateContext(playDevice, NULL);
    if (playContext)
    {
      if (!alcMakeContextCurrent(playContext)) {
        cout << "Error making context" << endl;
      }else
        cout << "Made context" << endl;
    }
  }else{
    cout << "Sound playback initialisation failed." << endl;
    check("initialising playback");
  }*/
}

void ogg_stream::setPitch(float p, bool setNow)
{
  pitch = p;
  if (setNow) alSourcef(source, AL_PITCH, pitch); // TODO binaural test
}

void ogg_stream::open(string path)
{
  int result;

  if(!(oggFile = fopen(path.c_str(), "rb")))
    throw string("Could not open Ogg file.");

  if((result = ov_open(oggFile, &oggStream, NULL, 0)) < 0)
  {
    fclose(oggFile);

    throw string("Could not open Ogg stream. ") + errorString(result);
  }

  initSource();
}


void ogg_stream::initSource()
{
  vorbisInfo = ov_info(&oggStream, -1);
  vorbisComment = ov_comment(&oggStream, -1);

  if(vorbisInfo->channels == 1) {
    format = AL_FORMAT_MONO16;
    cout << "AL_FORMAT_MONO16" << endl;
  }else{
    format = AL_FORMAT_STEREO16;
    cout << "AL_FORMAT_STEREO16" << endl;
  }

  alGenBuffers(OGG_NUM_BUFFERS, buffers);
  check("generate buffers");
  cout << "generated buffers. buffers[0]: " << buffers[0] << endl;
  cout << "generated buffers. buffers[1]: " << buffers[1] << endl;
  alGenSources(1, &source);
  check("generate sources");
  cout << "generated source: " << source << endl;
  bufferIdx = 0;

  //ALfloat value = 0;
  //alGetSourcef(source, AL_ROLLOFF_FACTOR, &value);
  //cout << "rolloff: " << value << endl;

  alSource3f(source, AL_POSITION,        x, y, z);
  alSourcef(source, AL_PITCH, pitch); // TODO binaural test
  alSource3f(source, AL_VELOCITY,        0.0, 0.0, 0.0);
  alSource3f(source, AL_DIRECTION,       0.0, 0.0, 0.0); // if not zero, sound is directional

  alSourcef (source, AL_ROLLOFF_FACTOR,  2); // default 1
  alSourcef (source, AL_REFERENCE_DISTANCE, 50); // default 1 - clamp gain below this
  alSourcei (source, AL_SOURCE_RELATIVE, AL_TRUE);

  sourceInitialised = true;
  //cout << "Source initialised" << endl;
}



void ogg_stream::release()
{
  // according to the 1.1 spec, 'A playing source can be deleted – the source
  // will be stopped automatically and then deleted.'
  // However, this does not work. Even though there is a context, the error
  // returned is AL_INVALID_OPERATION. Stopping the source first solves this problem.
  // Possibly because this isn't a normal source (AL_STATIC), it's streaming.
  /*int state = 0;
  alGetSourcei(source, AL_SOURCE_STATE, &state);
  cout << "State is now: ";
  switch (state) {
    case AL_INITIAL:
      cout << "initial" << endl;
      break;
    case AL_PLAYING:
      cout << "playing" << endl;
      break;
    case AL_PAUSED:
      cout << "paused" << endl;
      break;
    case AL_STOPPED:
      cout << "stopped" << endl;
      break;
  }*/
  if (sourceInitialised) {
    alSourceStop(source);
    alDeleteSources(1, &source);
    check("delete sources");
    alDeleteBuffers(OGG_NUM_BUFFERS, &buffers[0]);
    check("delete buffers");

    ov_clear(&oggStream);
    sourceInitialised = false;
  }
}


void ogg_stream::display()
{
  cout
    << "version         " << vorbisInfo->version         << "\n"
    << "channels        " << vorbisInfo->channels        << "\n"
    << "rate (hz)       " << vorbisInfo->rate            << "\n"
    << "bitrate upper   " << vorbisInfo->bitrate_upper   << "\n"
    << "bitrate nominal " << vorbisInfo->bitrate_nominal << "\n"
    << "bitrate lower   " << vorbisInfo->bitrate_lower   << "\n"
    << "bitrate window  " << vorbisInfo->bitrate_window  << "\n"
    << "\n"
    << "vendor " << vorbisComment->vendor << "\n";

  for(int i = 0; i < vorbisComment->comments; i++)
    cout << "   " << vorbisComment->user_comments[i] << "\n";

  cout << endl;

  /*cout << "raw: " << ov_raw_total(&oggStream, -1) << endl;
  cout << "pcm: " << ov_pcm_total(&oggStream, -1) << endl;
  cout << "time: " << ov_time_total(&oggStream, -1) << endl;*/
}


string ogg_stream::getComment(char* name)
  // Goes through comments to find "name" e.g. title/artist
{
  if (vorbisComment == NULL) return "ERROR!";
  //if (vorbisComment->comments > 10) return "ERROR2!";

  char *comment;
  string str;
  for (int i = 0; i < vorbisComment->comments; i++) {
    comment = vorbisComment->user_comments[i];
    if (strstr(comment, name) != NULL) {
      comment = strchr(comment, '=');
      if (comment != NULL) {
        comment++; // next character
        str = comment;
        break;
      }
    }
  }
  return str;
}

string ogg_stream::getTitle()
{
  return getComment("title");
}

string ogg_stream::getArtist()
{
  return getComment("artist");
}

string ogg_stream::getGenre()
{
  return getComment("genre");
}

string ogg_stream::getDate()
{
  return getComment("date");
}

string ogg_stream::getAlbum()
{
  return getComment("album");
}

string ogg_stream::getTracknumber()
{
  return getComment("tracknumber");
}


bool ogg_stream::playback_binaural()
// hack for starting both streams simultaneously
{
  bool active = true;
  bool somethingGotBuffered = false;

  // add data into buffers - there may not be enough data left to fill them all
  bufferIdx = 0;
  for (int i = 0; i < OGG_NUM_BUFFERS; i++) {
    active = stream(buffers[bufferIdx]);

    if (active) {
      alSourceQueueBuffers(source, 1, &buffers[bufferIdx]);
      check("queue buffers for playback");

      bufferIdx++;
      if (bufferIdx > OGG_NUM_BUFFERS - 1) bufferIdx = 0;
      somethingGotBuffered = true;
    }else
      break; // stream has finished
  }

  if (somethingGotBuffered) {
    alSourcePlay(source);
    return true;
  }else{
    return false; // stream has finished
  }
}

bool ogg_stream::playback(bool reset)
{
  if(playing())
    return true;

  // TODO quick hack for binaural - start both sources paused
  //return true;

  // update checks if playback has stopped (i.e. reached end of queue before it's been added to
  // (e.g. CPU busy with other processes)). That will then unqueue what's on there (should be
  // OGG_NUM_BUFFERS) and add on the same amount to the end (unless it runs out of file)
  // but if you really want to reset things then you can do so here
  if (reset) {
    cout << "Playback calling empty" << endl;
    empty();
  }

  bool active = true;
  bool somethingGotBuffered = false;

  // add data into buffers - there may not be enough data left to fill them all
  bufferIdx = 0;
  for (int i = 0; i < OGG_NUM_BUFFERS; i++) {
    std::cout << "about to stream" << std::endl;
    active = stream(buffers[bufferIdx]);
    cout << "done stream" << endl;

    if (active) {
      cout << "aout to queue" << endl;
      cout << "source: " << source << " , bufferIdx: " << bufferIdx << ", buffer: " << buffers[bufferIdx] << endl;
      alSourceQueueBuffers(source, 1, &buffers[bufferIdx]);
      check("queue buffers for playback");
      cout << "done queue" << endl;

      bufferIdx++;
      if (bufferIdx > OGG_NUM_BUFFERS - 1) bufferIdx = 0;
      somethingGotBuffered = true;
    }else
      break; // stream has finished
  }

  std::cout << "Got here! about to aalsourcePlay"<< std::endl;
  if (somethingGotBuffered) {
    alSourcePlay(source);
    return true;
  }else{
    return false; // stream has finished
  }
}




bool ogg_stream::playing()
{
  ALenum state;

  alGetSourcei(source, AL_SOURCE_STATE, &state);

  return (state == AL_PLAYING);
}

void ogg_stream::setSourceId(int s)
{
  sourceId = s;
}

int ogg_stream::getSourceId()
{
  return sourceId;
}


void ogg_stream::setPosition(float nx, float ny, float nz)
{
  x = nx, y = ny, z = nz;
  if (sourceInitialised) {
    ALfloat sourcePosition[] = { x, y, z };
    alSourcefv(source, AL_POSITION, sourcePosition);
    check("set position");
  }
}

void ogg_stream::getPosition(float &nx, float &ny, float &nz)
{
  nx = x, ny = y, nz = z;
}

float ogg_stream::getX()
{
  return x;
}

void ogg_stream::setX(float nx)
{
  x = nx;
}

void ogg_stream::setSpeed(float sx, float sy, float sz)
{
  speedX = sx, speedY = sy, speedZ = sz;
  //if (sourceInitialised) {
  //ALfloat sourceVelocity[] = { speedX / 100, speedY / 100, speedZ / 100 };
  //alSourcefv(source, AL_VELOCITY, sourceVelocity);
  //check("set speed");
  //}
}

void ogg_stream::getSpeed(float &sx, float &sy, float &sz)
{
  sx = speedX, sy = speedY, sz = speedZ;
}

/*void ogg_stream::updateVelocity()
{
  ALfloat sourceVelocity[] = { speedX, speedY, speedZ };
  alSourcefv(source, AL_VELOCITY, sourceVelocity);
}*/

/*void ogg_stream::setSpeedX(float sx)
{
  speedX = sx;
}

void ogg_stream::setSpeedY(float sy)
{
  speedY = sy;
}

void ogg_stream::setSpeedZ(float sz)
{
  speedZ = sz;
}*/

float ogg_stream::getSpeedX()
{
  return speedX;
}

float ogg_stream::getSpeedY()
{
  return speedY;
}

float ogg_stream::getSpeedZ()
{
  return speedZ;
}

void ogg_stream::setPaused(bool b)
{
  paused = b;
}

bool ogg_stream::getPaused()
{
  return paused;
}

void ogg_stream::setMoving(bool b)
{
  moving = b;
}

bool ogg_stream::getMoving()
{
  return moving;
}

void ogg_stream::setHoming(bool b)
{
  homing = b;
}

bool ogg_stream::getHoming()
{
  return homing;
}

void ogg_stream::getTarget(float &tx, float &ty, float &tz)
{
  tx = targX, ty = targY, tz = targZ;
}

void ogg_stream::setTarget(float tx, float ty, float tz)
{
  targX = tx, targY = ty, targZ = tz;
}

void ogg_stream::setFriction(bool b)
{
  friction = b;
}

bool ogg_stream::getFriction()
{
  return friction;
}

bool ogg_stream::update()
  // If one or more of the buffers on the queue have been played, then add to the queue.
  // If playback stopped (e.g. didn't get time to add to queue in time for continuous playback)
  // then start playback again
{
  bool active = true;
  bool somethingGotBuffered = false;

  int numBuffersUnqueued = unqueuePlayedBuffers();

  for (int i = 0; i < numBuffersUnqueued; i++) {
    if (bufferIdx < 0 || bufferIdx > OGG_NUM_BUFFERS - 1) {
      cerr << "Error in update: bufferIdx out of range: " << bufferIdx << endl;
      bufferIdx = 0;
    }

    active = stream(buffers[bufferIdx]);

    if (active) {
      alSourceQueueBuffers(source, 1, &buffers[bufferIdx]);
      check("queue buffers");

      bufferIdx++;
      if (bufferIdx > OGG_NUM_BUFFERS - 1) bufferIdx = 0;
      somethingGotBuffered = true;
    }else
      break; // stream has finished
  }

  // check it's still playing
  // if it's not, and there's no unqueued buffers, then this is no longer active
  // if it's not, and there's some unqueued buffers, then there more data may have been added
  bool stillPlaying = playing();

  // I suppose it's possible that after checking for unqueued buffers and before checking that
  // playback is still happening the buffers could be played and playback would stop.
  // If this is the case (i.e. there's some processed buffers left) then return true:
  // Next time these will be unqueued, and playback will be started again if more is added.
  int processed = 0;
  alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

  if (somethingGotBuffered || stillPlaying || processed) {
    if (!stillPlaying) {
      //cout << "Ogg stream interrupted for source " << sourceId << " - detected in update";
      //if (somethingGotBuffered) cout << " - something got buffered";
      //cout << " - could have just started playback" << endl;
      alSourcePlay(source);
    }
    return true; // still active
  }else{
    return false;
  }
}


void ogg_stream::move(double amount)
{
  double time = ov_time_tell(&oggStream);
  time -= amount;
  ov_time_seek(&oggStream, time);
}

double ogg_stream::time_total()
{
  return ov_time_total(&oggStream, -1); // -1 gives total seconds of content (see vorbisfile.c)
}

double ogg_stream::tell()
{
  return ov_time_tell(&oggStream);
}

void ogg_stream::seek(double time)
{
  ov_time_seek(&oggStream, time);
}


bool ogg_stream::stream(ALuint buffer)
  // returns true if some of stream has been added to buffer,
  // or false if stream has ended
{
  //static int count = 0;
  //if (count > 9) return true;
  //cout << "next stream" << endl;
  char pcm[OGG_BUFFER_SIZE];
  int  size = 0;
  int  section;
  int  result;

  while(size < OGG_BUFFER_SIZE)
  {
    cout << "about to ov_read" << endl;
    result = ov_read(&oggStream, pcm + size, OGG_BUFFER_SIZE - size, 0, 2, 1, &section);
    cout << "done ov_read" << endl;

    if(result > 0)
      size += result;
    else
      if(result < 0)
        throw errorString(result);
      else
        break;
  }

  if(size == 0)
    return false;

  // for testing
  //if (count < 10) {
    cout << "about to alBufferData" << endl;
    alBufferData(buffer, format, pcm, size, vorbisInfo->rate);
    check("buffer data");
  //}
  //count++;

  return true;
}




int ogg_stream::unqueuePlayedBuffers()
{
  int processed = 0;

  alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

  // MSVC++ cannot allocate arrays dynamically on the stack
  // e.g. ALuint buffers[processed], so do one at a time!
  for (int i = 0; i < processed; i++) {
    ALuint buffer;

    alSourceUnqueueBuffers(source, 1, &buffer);
    check("unqueue played buffers");
  }

  return processed;
}


void ogg_stream::empty()
{
  // alSourceUnqueueBuffers() sometimes gives an AL_INVALID_VALUE error
  // (i.e. cannot unqueue source because it has not been processed)
  // even though source is stopped first (according to spec, the entire
  // queue should be considered processed if the source is stopped - perhaps
  // this only works when everything in the queue finishes playing and the
  // source automatically enters the AL_STOPPED state).

  // if the queue is not emptied properly the sound crackles
  // (to do with adding to the end of the queue and then restarting playback with
  // dead/already processed stuff at the beginning?)

  // workaround - delete the source

  // according to the 1.1 spec, 'A playing source can be deleted – the source
  // will be stopped automatically and then deleted.', although this doesn't work
  // (see other alDeleteSources)
  alSourceStop(source);
  alDeleteSources(1, &source);
  check("delete source from empty method");
  alGenSources(1, &source);
  check("generate source from empty method");

  /*int queued = 0;

  alSourceStop(source);
  int state = 0;
  alGetSourcei(source, AL_SOURCE_STATE, &state);
  cout << "Source stopped. State is now: ";
  switch (state) {
    case AL_INITIAL:
      cout << "initial" << endl;
      break;
    case AL_PLAYING:
      cout << "playing" << endl;
      break;
    case AL_PAUSED:
      cout << "paused" << endl;
      break;
    case AL_STOPPED:
      cout << "stopped" << endl;
      break;
  }

  alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

  cout << "Empty: Number of buffers on queue before process: " << queued << endl;

  while (queued--) {
    cout << "Empty: removing buffer from queue." << endl;
    ALuint buffer;
    alSourceUnqueueBuffers(source, 1, &buffer);
    check("empty");
  }

  alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
  check("empty2");
  cout << "Empty: Number of buffers on queue after process: " << queued << endl;
  if (queued != 0) {
    cerr << "empty: still buffers on queue!" << endl;
  }*/
}


bool ogg_stream::getSourceInitialised()
  // Check if the source has been opened/initialised (i.e. source is generated and can be released)
{
  return sourceInitialised;
}



// check for an error and pass a string to say where we are in the program
// errorMaker - what made the error?
int ogg_stream::check(string errorMaker)
{
  int error = alGetError();

  if(error != AL_NO_ERROR) {
    cout << "OpenAL error: ";
    switch (error) {
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
        cout << error << endl;
        break;
    }
    cout << "Occurred at: " << errorMaker << endl;
  }
  return error;
}



string ogg_stream::errorString(int code)
{
  switch(code)
  {
    case OV_EREAD:
      return string("Read from media.");
    case OV_ENOTVORBIS:
      return string("Not Vorbis data.");
    case OV_EVERSION:
      return string("Vorbis version mismatch.");
    case OV_EBADHEADER:
      return string("Invalid Vorbis header.");
    case OV_EFAULT:
      return string("Internal logic fault (bug or heap/stack corruption.");
    default:
      return string("Unknown Ogg error.");
  }
}


/*void ogg_stream::killAL()
{
  if (playDevice) { // if failed to open then don't attempt to close cos it'll segfault
    alcMakeContextCurrent(NULL);
    alcDestroyContext(playContext);
    alcCloseDevice(playDevice);
    cout << "Closed play device" << endl;
  }
}*/
