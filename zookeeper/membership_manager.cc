#include "cpp-base/zookeeper/membership_manager.h"

#include <glog/logging.h>
#include "cpp-base/string/join.h"

using std::string;
using std::vector;

namespace cpp_base {

MembershipManager::MembershipManager(const string& zk_host_port_list,
                                     const string& membership_management_zk_path,
                                     int num_active_server_positions,
                                     const string& my_service_address)
                                     : zk_client_(zk_host_port_list),
                                       zk_path_(membership_management_zk_path),
                                       num_active_servers_(num_active_server_positions),
                                       my_position_(kPositionBackupServer),
                                       my_service_address_(my_service_address),
                                       address_map_(num_active_server_positions) {}

MembershipManager::~MembershipManager() {}

bool MembershipManager::Init() {
    // Connect to ZooKeeper:
    if (!zk_client_.Connect(kZkConnecttionSetupTimeoutMillis)) {
        LOG(ERROR) << "Cannot continue; failed to connect to ZooKeeper";
        return false;
    }
    LOG(INFO) << "Successfully connected to ZK";

    // Make sure the path that we should work under either exists or is created:
    if (!MakeZkPathIfNotExists(zk_path_)) {
        LOG(ERROR) << "Path " << zk_path_ << " neither exists nor can be created";
        return false;
    }

    // Try to acquire one of the positions.
    int position;
    if (!FindMyPosition(3, &position)) {
        LOG(ERROR) << "Error taking a position";
        return false;
    }
    my_position_.store(position);
    LOG(INFO) << "I acquired position " << position << " (-1 would mean I'm a backup server)";

    // Find out who is behind every position.
    bool all_taken = false;
    while (!all_taken) {
        if (!RefreshAddressMap(3)) {
            LOG(ERROR) << "Error filling in the address map";
            return false;
        }
        // If not all positions are taken, wait until they are.
        all_taken = true;
        for (int pos = 0; pos < num_active_servers_; ++pos) {
            MutexLock lock(&address_map_mutex_);
            if (address_map_[pos].empty()) {
                LOG(INFO) << "Waiting until some server picks up position " << pos << " ...";
                all_taken = false;
                sleep(1);
                break;
            }
        }
    }
    LOG(INFO) << "All colleagues are up. Initialization done.";

    inited_ = true;
    return true;
}

string MembershipManager::AddressOfServer(int position) {
    CHECK(inited_);
    CHECK_GE(position, 0);
    CHECK_LT(position, num_active_servers_);
    MutexLock lock(&address_map_mutex_);
    return address_map_[position];
}

bool MembershipManager::MakeZkPathIfNotExists(string zk_path) {
    // From "path/to/node" extract "path" and "path/to" and "path/to/node".
    // Only cut at interior slashes, not the possible '/' at the beginning or end of 'path'.
    vector<int> slash_positions;
    for (int i = 1; i < zk_path.length() - 1; ++i) {
        if (zk_path[i] == '/')
            slash_positions.push_back(i);
    }
    vector<string> all_paths;
    for (int pos : slash_positions) {
        all_paths.push_back(zk_path.substr(0, pos));
    }
    all_paths.push_back(zk_path);

    // Create all the paths from top to bottom, if they don't exist.
    for (const string& p : all_paths) {
        auto ret = zk_client_.CreateNode(p, "", false, false);
        if (ret != ZooKeeperClient::NodeCreationResult::CREATED &&
            ret != ZooKeeperClient::NodeCreationResult::EXISTS) {
            LOG(ERROR) << "This node can't exist: " << p;
            return false;
        }
    }
    return true;
}

bool MembershipManager::FindMyPosition(int max_retries, int* acquired_position) {
    for (int pos = 0; pos < num_active_servers_; ) {
        // Compete for position 'pos'.
        string path = LeaderNodePathForPosition(zk_path_, pos);  // e.g. "a/b/c/leader-2"
        bool existed;
        string existing_address;
        if (!zk_client_.TryCreatingNode(path, my_service_address_, &existed, &existing_address)) {
            // Something went wrong. Need to retry acquiring 'position'.
            LOG(INFO) << "Error in acquiring position " << pos << "; retries left: " << max_retries;
            if (max_retries-- <= 0) {
                return false;
            }
            sleep(1);  // Allow ZK client to reconnect, if it's a connection problem
            continue;
        }

        if (!existed) {  // I won
            LOG(INFO) << "I created ZK node " << path << ":" << my_service_address_;
            *acquired_position = pos;
            return true;
        }
        LOG(INFO) << "ZK node " << path << " already exists with data " << existing_address << "'";

        // Node exists for this position, but there is a corner case where the server address
        // written under the node is actually mine: I died and came back, but the ephemeral node
        // takes time to go away. Instead of waiting, just delete the node and recreate my own.
        if (existing_address.compare(my_service_address_) == 0) {
            LOG(INFO) << "ZK node " << path << " exists and points to my own address,"
                      << " possibly from a previous run of mine. Deleting it ...";
            bool deleted;
            if (!zk_client_.DeleteNode(path, &deleted)) {
                LOG(ERROR) << "Weird situation: node " << path << " exists with my address ("
                           << my_service_address_ << ") under it, but I didn't create it, "
                           << "and I can't delete it either!";
                return false;
            }
            continue;
        }
        ++pos;  // Run for the next possible position
    }
    // Lost in all competitions! I'm a backup server then.
    *acquired_position = kPositionBackupServer;
    return true;
}

bool MembershipManager::RefreshAddressMap(int max_retries) {
    for (int pos = 0; pos < num_active_servers_; ++pos) {
        string path = LeaderNodePathForPosition(zk_path_, pos);
        bool exists;          // whether ZK node exists
        string address;  // host:port address of server behind position 'pos'
        if (!zk_client_.GetNode(path, &exists, &address)) {
            LOG(ERROR) << "Error getting ZK node " << path << "; retries left: " << max_retries;
            if (max_retries-- <= 0)
                return false;
        }

        // Let's do some error checking: if 'pos' is my own position:
        if (pos == MyPosition()) {
            if (!exists) LOG(ERROR) << "Inconsistency! exists=false";
            if (address.compare(my_service_address_)) LOG(ERROR) << "Inconsistency! " << address;
        }
        if (!exists) {
            LOG(INFO) << "No server has taken position " << pos;
            address = "";
        }
        MutexLock lock(&address_map_mutex_);
        address_map_[pos] = address;
    }
    return true;
}

// static
string MembershipManager::LeaderNodePathForPosition(const string& parent_path, int position) {
    // There is a slight chance that parent_path ends with a '/'.
    return (parent_path[parent_path.length() - 1] != '/')
                    ? StrCat(parent_path, "/leader-", position)
                    : StrCat(parent_path, "leader-", position);
}

}  // namespace cpp_base
