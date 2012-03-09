#include "source.h"
#include "soundloaderogg.h"

Source::Source()
{
  loader = new SoundLoaderOgg;
}

Source::~Source()
{
  delete loader;
}

void Source::init(const char *filename)
{
  loader->init();
  loader->load(filename);

  buf = new char[4096]; // TODO choose a sensible size based on motlab experience
  buflen = 4096;
}

void Source::kill()
{
  loader->unload();
  loader->kill();
}

void Source::update()
{
  loader->read(buf, buflen); // TODO choose sensible size, based on motlab experience

  // pass to motlab/stream
}

