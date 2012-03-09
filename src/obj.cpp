#include "obj.h"

Obj::Obj()
{
  type = OBJ_NORMAL;
}

Obj::~Obj()
{
}

void Obj::init(int nsources)
{
  src = new Source[nsources];
}

void Obj::kill()
{
  delete [] src;
}

void Obj::sound()
{
}

void Obj::move()
{
}

