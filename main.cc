#include "ping.rpc.pb.h"
#include "ping_service.h"
#include "pw_log/log.h"
#include "pw_rpc/client.h"
#include "pw_rpc/server.h"
#include "pw_status/status.h"

// Channel output that forwards packets directly to the server.
class ClientToServerChannelOutput : public pw::rpc::ChannelOutput {
 public:
  ClientToServerChannelOutput() : pw::rpc::ChannelOutput("client_to_server") {}
  void set_server(pw::rpc::Server* server) { server_ = server; }
  pw::Status Send(pw::span<const std::byte> buffer) override {
    if (server_ == nullptr) return pw::Status::Unavailable();
    return server_->ProcessPacket(buffer);
  }
 private:
  pw::rpc::Server* server_ = nullptr;
};

// Channel output that forwards packets directly to the client.
class ServerToClientChannelOutput : public pw::rpc::ChannelOutput {
 public:
  ServerToClientChannelOutput() : pw::rpc::ChannelOutput("server_to_client") {}
  void set_client(pw::rpc::Client* client) { client_ = client; }
  pw::Status Send(pw::span<const std::byte> buffer) override {
    if (client_ == nullptr) return pw::Status::Unavailable();
    return client_->ProcessPacket(buffer);
  }
 private:
  pw::rpc::Client* client_ = nullptr;
};

int main() {
  PW_LOG_INFO("Starting pw_rpc minimal demo...");

  // 1. Set up loopback channel outputs
  ClientToServerChannelOutput client_to_server_output;
  ServerToClientChannelOutput server_to_client_output;

  // 2. Create channels. We use Channel ID 1 for both.
  pw::rpc::Channel client_channels[] = {
      pw::rpc::Channel::Create<1>(&client_to_server_output)};
  pw::rpc::Channel server_channels[] = {
      pw::rpc::Channel::Create<1>(&server_to_client_output)};

  // 3. Instantiate Client and Server
  pw::rpc::Client client(client_channels);
  pw::rpc::Server server(server_channels);

  // Link outputs to their targets
  client_to_server_output.set_server(&server);
  server_to_client_output.set_client(&client);

  // 4. Register the service with the server
  PingService ping_service;
  server.RegisterService(ping_service);

  // 5. Instantiate the service client
  rpc::ping::pw_rpc::nanopb::PingService::Client ping_client(client, 1);

  // 6. Make the call
  rpc_ping_PingRequest request = rpc_ping_PingRequest_init_default;
  snprintf(request.value, sizeof(request.value), "Hello Pigweed RPC!");

  PW_LOG_INFO("Client sending ping...");
  
  bool callback_called = false;
  auto call = ping_client.Ping(
      request,
      [&callback_called](const rpc_ping_PingResponse& response, pw::Status status) {
        PW_LOG_INFO("Client received response status: %s", status.str());
        PW_LOG_INFO("Client received response value: %s", response.value);
        callback_called = true;
      });

  if (!callback_called && !call.active()) {
    PW_LOG_ERROR("Failed to initiate call");
    return 1;
  }

  if (!callback_called) {
    PW_LOG_ERROR("Callback was not called (should have been synchronous in loopback)");
    return 1;
  }

  PW_LOG_INFO("Demo finished successfully!");
  return 0;
}
