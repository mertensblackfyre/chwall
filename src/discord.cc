
#include "include/discord.h"
#include <cstdint>
#include <cstdlib>


int main() {

    UnixSocket sock;
   bool err =sock.create();

   if (err == false) {

    return EXIT_FAILURE;
   }

  std::string js = fmt::format(R"({{"v": 1, "client_id": "{}"}})", sock.client_id);

  Packet *packet = (Packet *)calloc(1,sizeof(Packet));
  packet->op = (uint32_t)Opcode::Handshake;
  packet->payload = js;
  packet->len = js.size();
  sock.send(packet);

  sock.read();


  free(packet);
  return 0;
}
