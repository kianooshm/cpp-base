#ifndef CPP_BASE_ZOOKEEPER_ZK_CLIENT_H_
#define CPP_BASE_ZOOKEEPER_ZK_CLIENT_H_

#define THREADED
#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper.jute.h>

#include <atomic>
#include <string>

namespace cpp_base {

class ZooKeeperClient {
  public:
    enum class ConnectionStatus {
        CONNECTING = 0, CONNECTED, DISCONNECTED,
    };
    enum class NodeCreationResult {
        ERROR = 0, CREATED, EXISTS,
    };

    // zk_host_port_list: comma-separated list of host:port addresses.
    ZooKeeperClient(const std::string& zk_host_port_list);
    ~ZooKeeperClient();

    // These are sync calls and block until either dis/re/connected to ZK or an error occurs.
    bool Connect(int timeout_ms);
    bool Disconnect();
    bool Reconnect(int timeout_ms);
    ConnectionStatus GetConnectionStatus();

    // Creates a ZK node with the given path, value and ZK flags.
    NodeCreationResult CreateNode(const std::string& node_path,
                                  const std::string& node_value,
                                  bool make_ephemeral,
                                  bool make_sequential);

    // Returns false on connection errors. Otherwise, returns true and sets 'exists' according to
    // whether the node existed. If it did, its value is filled in 'node_value'.
    bool GetNode(const std::string& node_path, bool* exists, std::string* node_value);

    // Returns false on connection errors. If node didn't exist, returns true and unsets 'existed'.
    bool DeleteNode(const std::string& node_path, bool* existed);

    // Competes for being the first to create the given ZK node using 'node_path' and 'node_value'.
    // Returns false on failures. Otherwise, returns true and sets 'existed' to false/true if
    // could/couldn't create the node (i.e. won the leadership of that node or lost). If the node
    // existed, its value is saved in 'existing_value'.
    bool TryCreatingNode(const std::string& node_path,
                         const std::string& node_value,
                         bool* existed,
                         std::string* existing_value);

    // Callback function passed to zookeeper_init(). Do not call anywhere else.
    void SesisonEventsCallback(zhandle_t* zh, int type, int state);

  private:
    const std::string zk_host_port_list_;
    zhandle_t* zhandle_ = nullptr;
    std::atomic<ConnectionStatus> connection_status_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_ZOOKEEPER_ZK_CLIENT_H_
