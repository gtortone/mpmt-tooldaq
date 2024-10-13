#include <string>
#include "ToolDAQChain.h"
#include "DataModel.h"

using namespace ToolFramework;

int main(int argc, char* argv[]){

  std::string conffile;
  if (argc==1)conffile="configfiles/Dummy/ToolChainConfig";
  else conffile=argv[1];

  DataModel* data_model = new DataModel();
  ToolDAQChain tools(conffile, data_model, argc, argv);

  return 0;
  
}
