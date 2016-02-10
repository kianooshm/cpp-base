#ifndef CPP_BASE_ZOOKEEPER_MEMBERSHIP_MANAGER_H_
#define CPP_BASE_ZOOKEEPER_MEMBERSHIP_MANAGER_H_

#include <string>
#include <vector>
#include "cpp-base/mutex.h"
#include "cpp-base/zookeeper/zk_client.h"

/*
    TODO:
    the connection itself needs care taking. If we are disconnected, we get an event that we went
    into CONNECTING state, but we stay in that stay (ZK retries) forever. we need a mechanism to
    bound that.

    unlike what we thought, disconnecting momentarily from ZK is rather *common*, so we do need
    a "come back" mechanism -- dont let someone take away the role with all its overheads, except
    if like >10 secs has passed.

    A WORKING server hast to:
        set a watch on all other active servers. also on its own connection state. upon a change,
        wait 10 sec and read all positions: who is behind each role.
    A BACKUP server:
        set a watch on all working servers. upon a change: compete for taking that position.
        it is the taken-over server's responsibility to figure that it's no longer active.
        servers don't talk to each other about role management -- only via ZK.

    there are a million corner cases. needs some careful testing to figure what's important and
    what not until reaching some reliable FT product.
*/

namespace cpp_base {

// Manages membership in a distributed cluster: N>0 active and K>=0 backup servers. Each server
// creates an instance of this class. It talks to ZooKeeper to find out which of the 0..N-1 active
// positions, or maybe a backup role, the server should take depending on what others have taken.
// Then, it continuously monitors which server is behind which active position, so the application
// can simply talk to "Server 5" without worrying if the server behind it has changed; indeed the
// application must beware that the switch may tak some time and must be prepared for transient
// errors.
class MembershipManager {
  public:
    // zk_host_port_list: comma-separated list of host:port addresses for the ZooKeeper cluster.
    // membership_management_zk_path: path in ZooKeeper under which to create membership nodes.
    // num_active_server_positions: Num active nodes in the cluster (excluding backup nodes).
    // my_service_address: my host:port address to which other servers can connect.
    MembershipManager(const std::string& zk_host_port_list,
                      const std::string& membership_management_zk_path,
                      int num_active_server_positions,
                      const std::string& my_service_address);
    ~MembershipManager();

    // Block until every active positions is taken by some server and the cluster is ready to work.
    // Once initialized, the acquired position in the cluster can be retrieved via MyPosition().
    bool Init();

    std::string AddressOfServer(int position);

    int NumActivePositions() const { return num_active_servers_; }
    int MyPosition() const { return my_position_.load(); }
    static const int kPositionBackupServer = -1;

  private:
    // Utility function to create a ZK path and its parents recursively, if any one does not exist.
    bool MakeZkPathIfNotExists(std::string zk_path);

    // Tries to acquire a positions from 0 to num_active_servers_-1. One and only one server will
    // acquire each position. If we got an active position, this function saves the position number
    // in 'acquired_position' and returns true. If it competed without errors but could not win any
    // (i.e. became a backup server), it saves kPositionBackupServer and returns true. If any error
    // occurred, a 'max_retries' number of retries will be made at 1-second intervals, and returns
    // false if problem persists.
    bool FindMyPosition(int max_retries, int* acquired_position);

    // Refreshes the address map in address_map_ vector: the host:port address of the server behind
    // every active position. Returns true/false on success/error.
    // Note: ZK node not existing is not considered an *error*. The address of that server position
    //       will be set to a blank string.
    // max_retries: on errors, retry this many times in 1-second intervals; meantime, if it's a ZK
    //              connection problem, ZK client will be trying to reconnect in the background.
    bool RefreshAddressMap(int max_retries);

    // Returns the path to ZK node that represents 'position', e.g., "a/b/c/leader-2".
    static std::string LeaderNodePathForPosition(const std::string& parent_path, int position);

    bool inited_ = false;
    ZooKeeperClient zk_client_;
    std::string zk_path_;  // Parent node in ZooKeeper for membership management nodes
    static const int kZkConnecttionSetupTimeoutMillis = 30000;  // 30 sec

    // Number of active servers in the cluster, excluding the backups.
    const int num_active_servers_;

    // If my position is in [0, num_active_servers-1], i'm an active member; otherwise a backup.
    std::atomic<int> my_position_;

    // My host:port address to declare via ZooKeeper so other servers can connect to it.
    const std::string my_service_address_;

    // The list of addresses (ip:port) behind active positions 0 to num_active_servers-1.
    std::vector<std::string> address_map_;
    Mutex address_map_mutex_;
};

}  // namespace cpp_base

#endif  // CPP_BASE_ZOOKEEPER_MEMBERSHIP_MANAGER_H_
