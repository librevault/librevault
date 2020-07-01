#include "Client.h"
#include "gui/MainWindow.h"

int main(int argc, char** argv) {
  Client client(argc, argv);
  return client.exec();
}
