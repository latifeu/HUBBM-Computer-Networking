//
// Created by alperen on 27.09.2023.
//

#include "Client.h"

Client::Client(string const& _id, string const& _ip, string const& _mac) {
    client_id = _id;
    client_ip = _ip;
    client_mac = _mac;
    in=false;
    out=false;
}

ostream &operator<<(ostream &os, const Client &client) {
    os << "client_id: " << client.client_id << " client_ip: " << client.client_ip << " client_mac: "
       << client.client_mac << endl;
    return os;
}

Client::~Client() {
    while (!incoming_queue.empty()) {
        std::stack<Packet*>& currentStack = incoming_queue.front();
        while (!currentStack.empty()) {
            delete currentStack.top();

            currentStack.pop();
        }
        incoming_queue.pop();
    }
    while (!outgoing_queue.empty()) {
        std::stack<Packet*>& currentStack = outgoing_queue.front();
        while (!currentStack.empty()) {
            delete currentStack.top();

            currentStack.pop();
        }
        outgoing_queue.pop();
    }

}