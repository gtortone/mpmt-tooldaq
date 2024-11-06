#include "Factory.h"
#include "Unity.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="HKMPMT") ret=new HKMPMT;
if (tool=="FPGAReadout") ret=new FPGAReadout;
if (tool=="NetTransmit") ret=new NetTransmit;
if (tool=="BoardControl") ret=new BoardControl;
  if (tool=="SlaveRunControl") ret=new SlaveRunControl;
return ret;
}
