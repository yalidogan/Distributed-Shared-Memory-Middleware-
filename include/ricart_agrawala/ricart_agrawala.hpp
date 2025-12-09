#include <vector>
#include <set>
#include <grpc_network.hpp>

enum class NodeState {
  RELEASED,
  WANTED,
  HELD
};

class RicartAgrawala {
public:
  RicartAgrawala(int node_id, GrpcNetwork& network);
  ~RicartAgrawala();

  // Lock Logic
  void request();
  void release();

  //Lamport Logical Clock
  void set_logical_clock(int sender_clock);

private:
  int node_id;
  int logical_clock;
  NodeState state;

  std::vector<int> deferred_queue;
  std::set<int> replied_nodes;
};