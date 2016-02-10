#include "cpp-base/zookeeper/zk_client.h"

#include <glog/logging.h>

using std::string;

namespace {

#define ENUM_CASE_TO_STRING(val, enum_case) \
        if ((val) == (enum_case)) { return #enum_case; }

const char* ZooWatchTypeString(int type) {
    ENUM_CASE_TO_STRING(type, ZOO_CREATED_EVENT)
    ENUM_CASE_TO_STRING(type, ZOO_DELETED_EVENT)
    ENUM_CASE_TO_STRING(type, ZOO_CHANGED_EVENT)
    ENUM_CASE_TO_STRING(type, ZOO_CHILD_EVENT)
    ENUM_CASE_TO_STRING(type, ZOO_SESSION_EVENT)
    ENUM_CASE_TO_STRING(type, ZOO_NOTWATCHING_EVENT)
    return "ZOO_UNKNOWN_EVENT_TYPE";
}

const char* ZooSessionStateString(int state) {
    ENUM_CASE_TO_STRING(state, ZOO_EXPIRED_SESSION_STATE)
    ENUM_CASE_TO_STRING(state, ZOO_AUTH_FAILED_STATE)
    ENUM_CASE_TO_STRING(state, ZOO_CONNECTING_STATE)
    ENUM_CASE_TO_STRING(state, ZOO_ASSOCIATING_STATE)
    ENUM_CASE_TO_STRING(state, ZOO_CONNECTED_STATE)
    return "ZOO_UNKNOWN_STATE";
}

const char* ZooErrorCodeString(int rc) {
    ENUM_CASE_TO_STRING(rc, ZOK)
    ENUM_CASE_TO_STRING(rc, ZSYSTEMERROR)
    ENUM_CASE_TO_STRING(rc, ZRUNTIMEINCONSISTENCY)
    ENUM_CASE_TO_STRING(rc, ZDATAINCONSISTENCY)
    ENUM_CASE_TO_STRING(rc, ZCONNECTIONLOSS)
    ENUM_CASE_TO_STRING(rc, ZMARSHALLINGERROR)
    ENUM_CASE_TO_STRING(rc, ZUNIMPLEMENTED)
    ENUM_CASE_TO_STRING(rc, ZOPERATIONTIMEOUT)
    ENUM_CASE_TO_STRING(rc, ZBADARGUMENTS)
    ENUM_CASE_TO_STRING(rc, ZINVALIDSTATE)
    ENUM_CASE_TO_STRING(rc, ZAPIERROR)
    ENUM_CASE_TO_STRING(rc, ZNONODE)
    ENUM_CASE_TO_STRING(rc, ZNOAUTH)
    ENUM_CASE_TO_STRING(rc, ZBADVERSION)
    ENUM_CASE_TO_STRING(rc, ZNOCHILDRENFOREPHEMERALS)
    ENUM_CASE_TO_STRING(rc, ZNODEEXISTS)
    ENUM_CASE_TO_STRING(rc, ZNOTEMPTY)
    ENUM_CASE_TO_STRING(rc, ZSESSIONEXPIRED)
    ENUM_CASE_TO_STRING(rc, ZINVALIDCALLBACK)
    ENUM_CASE_TO_STRING(rc, ZINVALIDACL)
    ENUM_CASE_TO_STRING(rc, ZAUTHFAILED)
    ENUM_CASE_TO_STRING(rc, ZCLOSING)
    ENUM_CASE_TO_STRING(rc, ZNOTHING)
    ENUM_CASE_TO_STRING(rc, ZSESSIONMOVED)
    //ENUM_CASE_TO_STRING(rc, ZNEWCONFIGNOQUORUM)
    //ENUM_CASE_TO_STRING(rc, ZRECONFIGINPROGRESS)
    return "ZOO_UNKNOWN_CODE";
}

#undef ENUM_CASE_TO_STRING

void SesisonEventsCallback(zhandle_t *zh, int type, int state, const char *path, void* ctx) {
    (reinterpret_cast<cpp_base::ZooKeeperClient*>(ctx))->SesisonEventsCallback(zh, type, state);
}

}  // namespace

namespace cpp_base {

ZooKeeperClient::ZooKeeperClient(const string& zk_host_port_list)
        : zk_host_port_list_(zk_host_port_list) {
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
}

ZooKeeperClient::~ZooKeeperClient() {
    if (connection_status_.load() != ConnectionStatus::DISCONNECTED)
        Disconnect();
}

bool ZooKeeperClient::Connect(int timeout_ms) {
    connection_status_.store(ConnectionStatus::CONNECTING);
    zhandle_ = zookeeper_init(zk_host_port_list_.c_str(),
                              ::SesisonEventsCallback,
                              timeout_ms,
                              nullptr,  // &client_id
                              this,     // context pointer
                              0);       // flags
    if (zhandle_ == nullptr) {
        LOG(ERROR) << "Cannot connect to any ZK node from " << zk_host_port_list_
                   << "; " << errno << ": " << strerror(errno);
        connection_status_.store(ConnectionStatus::DISCONNECTED);
        return false;
    }
    // Wait until the callback is called:
    while (connection_status_.load() == ConnectionStatus::CONNECTING) {
        usleep(1000);
    }
    return (connection_status_.load() == ConnectionStatus::CONNECTED);
}

bool ZooKeeperClient::Disconnect() {
    connection_status_.store(ConnectionStatus::DISCONNECTED);
    int ret = zookeeper_close(zhandle_);
    if (ret != ZOK) {
        LOG(WARNING) << "Could not close ZK connection: " << ret;
        return false;
    } else {
        LOG(INFO) << "Successfully closed ZK connection";
        return true;
    }
}

bool ZooKeeperClient::Reconnect(int timeout_ms) {
    Disconnect();
    LOG(INFO) << "Reconnecting to ZK ...";
    return Connect(timeout_ms);
}

ZooKeeperClient::ConnectionStatus ZooKeeperClient::GetConnectionStatus() {
    CHECK_NOTNULL(zhandle_);
    int state = zoo_state(zhandle_);

    if (state == ZOO_CONNECTED_STATE) {
        return ConnectionStatus::CONNECTED;
    } else if (state == ZOO_CONNECTING_STATE || state == ZOO_ASSOCIATING_STATE) {
        return ConnectionStatus::CONNECTING;
    }
    return ConnectionStatus::DISCONNECTED;
}

ZooKeeperClient::NodeCreationResult ZooKeeperClient::CreateNode(
        const string& node_path, const string& node_value,
        bool make_ephemeral, bool make_sequential) {
    CHECK_NOTNULL(zhandle_);
    if (connection_status_.load() != ConnectionStatus::CONNECTED) {
        LOG(WARNING) << "Cannot create ZK node: not connected to ZK";
        return ZooKeeperClient::NodeCreationResult::ERROR;
    }

    int flags = 0;
    if (make_ephemeral)  flags |= ZOO_EPHEMERAL;
    if (make_sequential) flags |= ZOO_SEQUENCE;

    int ret = zoo_create(zhandle_,
                         node_path.c_str(),
                         node_value.c_str(),
                         node_value.length(),
                         &ZOO_OPEN_ACL_UNSAFE,
                         flags,
                         nullptr,  // path buffer
                         0);       // path buffer len
    if (ret == ZOK) {
        return ZooKeeperClient::NodeCreationResult::CREATED;
    } else if (ret == ZNODEEXISTS) {
        return ZooKeeperClient::NodeCreationResult::EXISTS;
    }
    LOG(WARNING) << "Error creating ZK node " << node_path << ": " << ZooErrorCodeString(ret);
    return ZooKeeperClient::NodeCreationResult::ERROR;
}

bool ZooKeeperClient::GetNode(const string& node_path, bool* exists, string* node_value) {
    CHECK_NOTNULL(zhandle_);
    if (connection_status_.load() != ConnectionStatus::CONNECTED) {
        LOG(WARNING) << "Cannot retrieve ZK node: not connected to ZK";
        return false;
    }

    char buff[1024];
    int buff_len = sizeof(buff);
    int ret = zoo_get(zhandle_, node_path.c_str(), 0, &buff[0], &buff_len, nullptr);
    if (ret == ZOK) {
        *exists = true;
        *node_value = string(buff, buff_len);
        return true;
    } else if (ret == ZNONODE) {
        *exists = false;
        return true;
    }
    LOG(WARNING) << "Error retrieving ZK node " << node_path << ": " << ZooErrorCodeString(ret);
    return false;
}

bool ZooKeeperClient::DeleteNode(const std::string& node_path, bool* existed) {
    int ret = zoo_delete(zhandle_, node_path.c_str(), -1);
    if (ret == ZOK) {
        *existed = true;
        return true;
    }
    if (ret == ZNONODE) {
        *existed = false;
        return true;
    }
    LOG(WARNING) << "Error deleting ZK node " << node_path << ": " << ZooErrorCodeString(ret);
    return false;
}

bool ZooKeeperClient::TryCreatingNode(const string& node_path,
                                      const string& node_value,
                                      bool* existed,
                                      std::string* existing_value) {
    // Try to create the node. If successful, I won.
    NodeCreationResult res = CreateNode(node_path, node_value, true, false);
    if (res == NodeCreationResult::CREATED) {
        LOG(INFO) << "I took leadership for " << node_path;
        *existed = false;
        return true;
    }
    if (res == NodeCreationResult::EXISTS) {
        LOG(INFO) << "Someone else took leadership for " << node_path;
        bool ret = GetNode(node_path, existed, existing_value);
        if (ret && *existed) {
            return true;
        }
        LOG(WARNING) << "Couldn't create node " << node_path << " (exists), but can't retrieve it";
    } else {
        LOG(WARNING) << "Unexpected error when running for leadership at " << node_path;
    }
    return false;
}

void ZooKeeperClient::SesisonEventsCallback(zhandle_t* zh, int type, int state) {
    if (type != ZOO_SESSION_EVENT) {
        LOG(WARNING) << "Ignoring unexpected session callback: " << ZooWatchTypeString(type);
        return;
    }

    if (state == ZOO_CONNECTED_STATE) {
        LOG(INFO) << "Successfully connected to ZooKeeper";
        connection_status_.store(ConnectionStatus::CONNECTED);
        return;
    }

    LOG(WARNING) << "ZooKeeper session state = " << ZooSessionStateString(state);

    if (state == ZOO_CONNECTING_STATE || state == ZOO_ASSOCIATING_STATE) {
        connection_status_.store(ConnectionStatus::CONNECTING);
    } else {
        connection_status_.store(ConnectionStatus::DISCONNECTED);
    }
}

}  // namespace cpp_base
