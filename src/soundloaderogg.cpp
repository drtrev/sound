#include "soundloaderogg.h"
#include <iostream>
#include "vorbis/vorbisfile.h"

using std::cout;
using std::cerr;
using std::endl;

void SoundLoaderOgg::SoundLoaderOgg()
{
  bitstream = 0;
}

void SoundLoaderOgg::init()
{
}

void SoundLoaderOgg::kill()
{
}

void SoundLoaderOgg::load(const char *filename)
{
  if (ov_fopen(filename, vorbisFile) != 0) {
    cerr << "Error opening file: " << filename << endl;
  }
}

void SoundLoaderOgg::unload()
{
  ov_clear(file);
}

long SoundLoaderOgg::read(char *buf, int n)
{
  long ret = ov_read(vorbisFile, buf, n, 0, 2, 1, &bitstream);
  if (ret == OV_HOLE) cerr << "SoundLoaderOgg read error: OV_HOLE" << endl;
  if (ret == OV_EBADLINK) cerr << "SoundLoaderOgg read error: OV_EBADLINK" << endl;
  if (ret == OV_EINVAL) cerr << "SoundLoaderOgg read error: OV_EINVAL" << endl;

  return ret; // number of bytes read, or 0 for EOF
  // docs say bytes read will be generally less than length, because it opens max 1 vorbis packet per invocation
  // TODO see also ov_crosslap in vorbisfile
}

