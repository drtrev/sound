#ifndef SOUNDLOADER_H
#define SOUNDLOADER_H

class SoundLoader {
  private:
  protected:
  public:
    virtual void init();
    virtual void kill();
    virtual void load(const char *filename);
    virtual void unload(); // close file, clean up
    virtual long read(char *buf, int n); // returns bytes read
};

#endif
