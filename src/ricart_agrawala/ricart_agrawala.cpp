#include "ricart_agrawala.hpp"

RicartAgrawala::RicartAgrawala(int node_id)
    : node_id(node_id), logical_clock(0), state(NodeState::RELEASED){
}
RicartAgrawala::~RicartAgrawala() {
}


void RicartAgrawala::request() {
    logical_clock++;
    state = NodeState::WANTED;
    replied_nodes.clear();

    // Send request to all known peers
    for (int peer_id : known_peers) {
        // Construct and send request message
    }
}

void RicartAgrawala::release() {
    state = NodeState::RELEASED;

    // Reply to deferred requests
    for (int deferred_node : deferred_queue) {
        // Construct and send reply message 
    }
    deferred_queue.clear();
}

void RicartAgrawala::set_logical_clock(int sender_clock) {
    logical_clock = std::max(logical_clock, sender_clock) + 1;
}
