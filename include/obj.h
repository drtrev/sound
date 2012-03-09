#ifndef OBJ_H
#define OBJ_H

class Obj {
  private:
    struct Props {
      sound::Vector pos, oldPos, speed, targ, accel, oldAccel;
      bool moving, homing, frictionon, paused;
      float pitch, power, friction, minSpeed;

      Props() : moving(true), homing(false), frictionon(true), paused(false), pitch(1), power(800), friction(400), minSpeed(0.001) {}
    } props;

    enum ObjType { OBJ_NORMAL, OBJ_BINAURAL };
    ObjType type;

    Source *src;

  protected:

  public:
    Obj();
    ~Obj();

    void init();
    void kill();

    void sound(); // update sound playback in sources
    void move();  // physics update, can call at different frequency to sound update
};

#endif
