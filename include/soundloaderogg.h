#ifndef SOUNDLOADEROGG_H
#define SOUNDLOADEROGG_H

#include "soundloader.h"
#include <cstdio>

// an implementation of soundloader using Vorbisfile
class SoundLoaderOgg : SoundLoader {
  private:
    FILE* file;
    OggVorbis_File *vorbisFile;
    int bitstream;

  public:
    SoundLoaderOgg();
    void init();
    void kill();
    void load(const char *filename);
    void unload();
    long read(char *buf, int n);
};

#endif
