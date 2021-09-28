#include "ClientGui.h"
#include "gui/MainWindow.h"

int main(int argc, char** argv) {
  Q_INIT_RESOURCE(config);
  ClientGui client(argc, argv);
  return ClientGui::exec();
}
