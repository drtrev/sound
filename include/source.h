#ifndef SOURCE_H
#define SOURCE_H

#include "types.h"

class Source {
  private:
    sound::Vector pos; // can have its own position, independent of obj position, e.g. for binaural, multiple sources for one object
    SoundLoader *loader;
    char *buf;
    int buflen;

  protected:

  public:
    Source();
    ~Source();

    void init();
    void kill();

    void update();

};

#endif
