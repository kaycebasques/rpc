#include "ping_service.h"
#include "pw_log/log.h"
#include "pw_rpc_system_server/rpc_server.h"

int main() {
  // Initialize the system server. This sets up the socket and logging.
  pw::rpc::system_server::Init();

  // Register the PingService with the system server.
  PingService ping_service;
  pw::rpc::system_server::Server().RegisterService(ping_service);

  PW_LOG_INFO("Starting pw_rpc server...");
  
  // Start the server. This blocks forever, processing packets from the socket.
  if (const pw::Status status = pw::rpc::system_server::Start(); !status.ok()) {
    PW_LOG_ERROR("Failed to start rpc server: %s", status.str());
    return 1;
  }

  return 0;
}
